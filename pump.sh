#!/usr/bin/env bash
#
# pump.sh — cut and publish a new bspwm1 release.
#
# Decides the semver bump from the Conventional-Commit messages since the last
# tag, or takes an explicit level/version:
#
#   ./pump.sh            # auto: feat!/BREAKING -> major, feat -> minor, else patch
#   ./pump.sh patch      # force a patch bump   (x.y.Z)
#   ./pump.sh minor      # force a minor bump   (x.Y.0)
#   ./pump.sh major      # force a major bump   (X.0.0)
#   ./pump.sh 2.0.0      # set an explicit version
#
# It bumps VERSION + the PKGBUILD + .SRCINFO, builds and tests the tree,
# commits the bump, then (after one confirmation) pushes the tag and updates
# the AUR package.
#
#   -n, --dry-run   do everything except commit/push/AUR (prints the plan)
#   -y, --yes       skip the confirmation prompt before pushing
#
set -euo pipefail

# ---- options -----------------------------------------------------------------
DRY=0 ASSUME_YES=0 LEVEL=auto
for a in "$@"; do
	case "$a" in
		-n|--dry-run) DRY=1 ;;
		-y|--yes)     ASSUME_YES=1 ;;
		auto|patch|minor|major) LEVEL="$a" ;;
		[0-9]*.[0-9]*.[0-9]*)   LEVEL="$a" ;;
		*) echo "usage: $0 [auto|patch|minor|major|X.Y.Z] [-n] [-y]" >&2; exit 2 ;;
	esac
done

cd "$(git rev-parse --show-toplevel)"

say()  { printf '\033[1;36m==>\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31mpump: %s\033[0m\n' "$*" >&2; exit 1; }

# ---- preconditions -----------------------------------------------------------
branch=$(git rev-parse --abbrev-ref HEAD)
[ "$branch" = master ] || die "not on master (on '$branch')"
[ -z "$(git status --porcelain --untracked-files=no)" ] || die "working tree has uncommitted changes"
[ -f VERSION ] || die "no VERSION file"

cur=$(tr -d '[:space:]' < VERSION)
[[ "$cur" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "VERSION '$cur' is not X.Y.Z"
IFS=. read -r MA MI PA <<<"$cur"

# ---- decide the bump ---------------------------------------------------------
last_tag=$(git describe --tags --abbrev=0 2>/dev/null || true)
range="${last_tag:+$last_tag..}HEAD"
log=$(git log --format='%s%n%b' $range 2>/dev/null || true)
[ -n "$(git log --oneline $range 2>/dev/null)" ] || die "no commits since ${last_tag:-the beginning}; nothing to release"

if [ "$LEVEL" = auto ]; then
	if grep -qiE '^[a-z]+(\([^)]*\))?!:' <<<"$log" || grep -qi 'BREAKING CHANGE' <<<"$log"; then
		LEVEL=major
	elif grep -qiE '^feat(\([^)]*\))?:' <<<"$log"; then
		LEVEL=minor
	else
		LEVEL=patch
	fi
fi

case "$LEVEL" in
	major) new="$((MA+1)).0.0" ;;
	minor) new="$MA.$((MI+1)).0" ;;
	patch) new="$MA.$MI.$((PA+1))" ;;
	*)     new="$LEVEL"; LEVEL="explicit" ;;
esac
[[ "$new" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "computed version '$new' is not X.Y.Z"
git rev-parse -q --verify "refs/tags/v$new" >/dev/null && die "tag v$new already exists"

say "release $cur -> $new  (bump: $LEVEL, since ${last_tag:-start})"
echo "--- commits in this release ---"
git log --oneline $range
echo "-------------------------------"

# ---- bump the version strings ------------------------------------------------
pkgbuilds=(pkg/arch/PKGBUILD)
srcinfos=(pkg/arch/.SRCINFO)

printf '%s\n' "$new" > VERSION
sed -i "s/^pkgver=${cur//./\\.}\$/pkgver=$new/" "${pkgbuilds[@]}"
sed -i "s/pkgver = ${cur//./\\.}/pkgver = $new/; s/tag=v${cur//./\\.}/tag=v$new/" "${srcinfos[@]}"

# verify every file actually moved to the new version
for f in VERSION "${pkgbuilds[@]}" "${srcinfos[@]}"; do
	grep -q "$new" "$f" || die "failed to bump $f"
done
say "bumped VERSION, PKGBUILDs, .SRCINFOs"

# ---- build + test ------------------------------------------------------------
say "building + testing"
make clean >/dev/null 2>&1 || true
make >/dev/null 2>&1       || die "build failed"
make test >/dev/null 2>&1  || die "tests failed (run: make test)"
say "build green"

if [ "$DRY" = 1 ]; then
	say "dry run — reverting version bump, nothing committed or pushed"
	git checkout -- VERSION "${pkgbuilds[@]}" "${srcinfos[@]}"
	exit 0
fi

# ---- commit ------------------------------------------------------------------
changelog=$(git log --format='- %s' $range | grep -vE '^- release:' || true)
git add VERSION "${pkgbuilds[@]}" "${srcinfos[@]}"
git commit -q -F - <<EOF
release: v$new

$changelog

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
say "committed release v$new"

# ---- confirm before anything leaves this machine ----------------------------
if [ "$ASSUME_YES" != 1 ]; then
	printf '\033[1;33mPush v%s to origin + the AUR package? [y/N] \033[0m' "$new"
	read -r reply
	case "$reply" in
		y|Y|yes) ;;
		*) say "stopped before push. Commit is in place; run 'git reset --soft HEAD~1' to undo, or push manually."; exit 0 ;;
	esac
fi

# ---- push + tag --------------------------------------------------------------
say "pushing master + tag v$new"
git push origin master
git tag -a "v$new" -m "v$new"
git push origin "v$new"

# ---- AUR ---------------------------------------------------------------------
aur_update() {
	local pkg="$1" dir src
	dir="$(mktemp -d "${TMPDIR:-/tmp}/aur-$pkg.XXXXXX")"
	say "AUR: $pkg"
	git clone -q "ssh://aur@aur.archlinux.org/$pkg.git" "$dir"
	src=pkg/arch
	cp "$src/PKGBUILD" "$src/.SRCINFO" "$dir/"
	( cd "$dir" && git add -A && git commit -q -m "v$new" && git push -q )
	rm -rf "$dir"
}
aur_update bspwm1

say "released v$new. Install with: yay -Sy bspwm1   then: bspc wm -r"
