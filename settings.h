#define CONFIG_FILE         "bspwmrc"
#define BORDER_WIDTH        1
#define INNER_BORDER_WIDTH  2

#define SPLIT_RATIO         0.5
#define SMART_SURROUNDINGS  true

#define NORMAL_BORDER_COLOR  "#333333"
#define ACTIVE_BORDER_COLOR  "#DDDDDD"
#define INNER_BORDER_COLOR   "#111111"

char *normal_border_color;
char *active_border_color;
char *inner_border_color;

int border_width;
int inner_border_width;
bool smart_surroundings;
double split_ratio;

void load_settings(void);
