
#ifndef  _APA102_H_H___H___
#define  _APA102_H_H___H___


#ifdef __cplusplus
extern "C"{
#endif

typedef void * apa102_hanle;

int  apa102_set_pixel_rgb(apa102_hanle handle,int led_num, uint32_t rgb_color, int bright_percent);

int  apa102_set_pixel(apa102_hanle handle,int led_num, int red, int green, int blue, int bright_percent);

int apa102_show(apa102_hanle handle);

apa102_hanle   apa102_open(int led_num);

void apa102_close(apa102_hanle handle);

int apa102_clear_strip(apa102_hanle handle);


#ifdef __cplusplus
}
#endif

#endif
