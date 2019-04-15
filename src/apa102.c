
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <wiringPiSPI.h>

#include <arpa/inet.h>
#include <errno.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "apa102.h"
#include "duerapp_config.h"

#define MAX_BRIGHTNESS          31

#define  DEFAULT_SPI_MAX_SPEED_HZ    8000000
static const uint8_t  LED_START = 0b11100000;

typedef struct _APA102_PIXEL
{
	    int    bright_percent;
	    int    red;
	    int    green;
	    int    blue;
}APA102_PIXEL;

typedef struct _APA102
{
	     uint8_t  led_start;
	    int    led_num;
	    int    spi_channel;
	    uint32_t spi_max_speed_hz;
	    uint32_t  global_brightness;
	    APA102_PIXEL  *pixels;
}APA102;

apa102_hanle   apa102_open(int led_num)
{
	APA102 * dev = (APA102*)malloc(sizeof(APA102));
	int i;
	
	if(dev==NULL)
	{ 
		   return NULL;
	}
	
	bzero(dev,sizeof(APA102));
	dev->led_num = led_num;
	dev->spi_channel = 0;
	dev->pixels = (APA102_PIXEL*)malloc(sizeof(APA102_PIXEL)*led_num);
	if(dev->pixels==NULL)
	{
	       	return NULL;
	}
	
	bzero(dev->pixels,sizeof(APA102_PIXEL)*led_num);
	for(i=0;i<led_num;i++)
	{
		  dev->pixels[i].bright_percent = 50;
	}
	
	dev->global_brightness =  MAX_BRIGHTNESS;
	if(wiringPiSPISetup (dev->spi_channel ,DEFAULT_SPI_MAX_SPEED_HZ)<0)
	{
		    DUER_LOGE("setup spi fail!\n");
		   free(dev);
	       return NULL;	
	}
	
	return (apa102_hanle)dev;
}

void apa102_close(apa102_hanle handle)
{
	APA102 * dev = (APA102 *)handle;
	
	if(dev==NULL)
		return;
		
	apa102_clear_strip(handle);	
	if(dev->pixels!=NULL)
		free(dev->pixels);
	
	free(dev);
		
	return ;
}

static int apa102_clock_start_frame(APA102 *dev)
{
	    uint8_t data[4] = {0,0,0,0};
	    
	    wiringPiSPIDataRW(dev->spi_channel ,data,4);
	    return 0;
}

static int apa102_clock_end_frame(APA102 *dev)
{
	uint8_t  zero=0;
	int i;
	
	for(i=0;i<(dev->led_num+15)/16;i++)
	{
		wiringPiSPIDataRW(dev->spi_channel ,&zero,1);
	}
	
	return 0;
}

int  apa102_set_pixel(apa102_hanle handle,int led_num, int red, int green, int blue, int bright_percent)
{
	APA102 * dev = (APA102 *)handle;
	
	if(dev==NULL)
		return -1;
	
	if(led_num<0||led_num>=dev->led_num)
	{
		DUER_LOGE("invalid led num!\n");	
		return -1;
	}
	
	dev->pixels[led_num].bright_percent = bright_percent;
	dev->pixels[led_num].blue = blue;
	dev->pixels[led_num].green  = green;
	dev->pixels[led_num].red = red;
		
	return 0;
}

int  apa102_set_pixel_rgb(apa102_hanle handle,int led_num, uint32_t rgb_color, int bright_percent)
{
	int red,green,blue;
	     
	red = (rgb_color>>16)&0xFF;
        green =(rgb_color>>8)&0xFF;
	blue = (rgb_color)&0xFF;
	
	return  apa102_set_pixel(handle,led_num,red,green,blue,bright_percent);
}

int apa102_show(apa102_hanle handle)
{
	   APA102 * dev = (APA102 *)handle;
	   int   i;
	   uint8_t  *leds_data;
	   uint8_t led_start;
	   double  brightness=0;
	    
	   leds_data = (uint8_t*)malloc(dev->led_num*4);
	   
	   if(leds_data==NULL)
		return -1;
	   		
	   for(i=0;i<dev->led_num;i++)
	   {
		      uint8_t   ibrightness =0;
		      
		      brightness = (dev-> pixels[i] .bright_percent*dev->global_brightness) /100.0;
		      ibrightness = brightness;	
		      led_start = (ibrightness & 0b00011111) | LED_START;
		      		   
		      leds_data[4*i] = led_start;
		      leds_data[4*i+1] = dev-> pixels[i].red;
		      leds_data[4*i+2] = dev-> pixels[i].green;
		      leds_data[4*i+3] = dev-> pixels[i].blue;
	    }
	     	
	   apa102_clock_start_frame(dev);
	   wiringPiSPIDataRW(dev->spi_channel,leds_data,4*dev->led_num);
	   apa102_clock_end_frame(dev);
	   
	   free(leds_data);
	   
	   return 0;
}

int apa102_clear_strip(apa102_hanle handle)
{
	int  i;
	APA102 * dev = (APA102 *)handle;
	
	if(dev==NULL)
		return -1;
	
	for(i=0;i<dev->led_num;i++)
	{
		apa102_set_pixel_rgb(handle,i,0,0);
	}
	
	apa102_show(handle);
	
	return 0;
}

int apa102_test(void)
{
	static apa102_hanle  apa102_led_dev=NULL ;
	
	if(apa102_led_dev==NULL)
	{
		apa102_led_dev =  apa102_open(3);
	}
	
	if(apa102_led_dev==NULL)
	{
		DUER_LOGE("apa102 led open fail!\n");
		return -1;
	}
	
	apa102_set_pixel_rgb(apa102_led_dev,0,0xff0000,100);
	apa102_set_pixel_rgb(apa102_led_dev,1,0x00ff00,10);
	apa102_set_pixel_rgb(apa102_led_dev,2,0x0000ff,20);
		
	apa102_show(apa102_led_dev);
	
	return    0;
}


