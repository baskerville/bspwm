import Text.Pandoc
import Data.Char (toUpper)

main :: IO ()
main = toJsonFilter (capitalizeHeaders . bottomUp delink)

capitalizeHeaders :: Block -> Block
capitalizeHeaders (Header 1 attr xs) = Header 1 attr $ bottomUp capitalize xs
capitalizeHeaders x                  = x

capitalize :: Inline -> Inline
capitalize (Str xs) = Str $ map toUpper xs
capitalize x        = x

delink :: [Inline] -> [Inline]
delink ((Link txt _) : xs) = txt ++ delink xs
delink ((Image _ _) : xs)  = []
delink (x : xs)            = x : delink xs
delink []                  = []
