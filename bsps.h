#ifndef _BSPS_H
#define _BSPS_H

void setup(void);
void register_events(void);
void handle_signal(int);
void update_wintitle(void);
void update_deskname(void);
void output_infos(void);

#endif
