
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
#include "lightduer_timers.h"
#include "duerapp_config.h"
#include "apa102.h"
#include "led.h"

static apa102_hanle  				s_apa102led_dev=NULL;
#define LED_NUM							(3)

static int  s_cur_led_mode=0;
static duer_timer_handler    s_led_timer=NULL;

int led_set_flash0(void);

void  led_timer_callback(void  *arg)
{
	    if(s_cur_led_mode = LED_MODE_FLASH0)
	    {
				led_set_flash0();
		}
		return ;
}

int  led_init(void)
{
	  if(s_apa102led_dev!=NULL)
			return 0;
	   
	  s_apa102led_dev = apa102_open(LED_NUM);
	  
	  if(s_apa102led_dev==NULL)
			return -1;
	 
	 s_led_timer = duer_timer_acquire(led_timer_callback,NULL,DUER_TIMER_PERIODIC);
	 if(s_led_timer==NULL)
	 {
		    DUER_LOGE("create timer fail!\n");
			return -1;
	 }
	 	
	 return 0;
}

int led_set_red(void)
{
		int  i=0;
		
		if(s_apa102led_dev==NULL)
		{
			   return -1;
		}
			
		for(i=0;i<LED_NUM;i++)
		{
			  	apa102_set_pixel_rgb(s_apa102led_dev,i,0x0000ff,30);
		}
		
		apa102_show(s_apa102led_dev);
		
		return 0;
}

int led_set_green(void)
{
		int  i=0;
		
		if(s_apa102led_dev==NULL)
		{
			   return -1;
		}
			
		for(i=0;i<LED_NUM;i++)
		{
			  	apa102_set_pixel_rgb(s_apa102led_dev,i,0x00ff00,30);
		}
		apa102_show(s_apa102led_dev);
		
		return 0;
}

int led_set_blue(void)
{
		int  i=0;
		 
		if(s_apa102led_dev==NULL)
		{
			   return -1;
		}
		
		for(i=0;i<LED_NUM;i++)
		{
			  	apa102_set_pixel_rgb(s_apa102led_dev,i,0xff0000,50);
		}
		
		apa102_show(s_apa102led_dev);
		
		return 0;
}

int led_set_flash0(void)
{
		int  i=0;
		static  int bright_percent = 60;
		static   int  step=-5;
		static  int index=0;
		int  rgb_map[3][LED_NUM] = {{0x0000ff,0x00ff00,0xff0000},{0xff0000,0x0000ff,0x00ff00},{0x00ff00,0xff0000,0x0000ff}};
		//printf("flash0\n");
		if(s_apa102led_dev==NULL)
		{
			   return -1;
		}
		
		if(bright_percent<=10)
		{
			   step = 10;
		}	
		
		if(bright_percent>=60)
		{
				step = -10;	
		}
		
		for(i=0;i<LED_NUM;i++)
		{
			  	apa102_set_pixel_rgb(s_apa102led_dev,i,rgb_map[index][i],bright_percent);
		}
		index++;
		if(index>=LED_NUM)
			index=0;
			
		apa102_show(s_apa102led_dev);
		bright_percent += step;
		
		return 0;
}

int led_set_mode(int mode)
{
		s_cur_led_mode = mode;
		
		duer_timer_stop(s_led_timer);
		switch(mode)
		{
				case   LED_MODE_OFF:
					led_off();
					break;
				case   LED_MODE_RED:
				     led_set_red();
					break;
				case  LED_MODE_GREEN:
					led_set_green();
					break;
				case  LED_MODE_BLUE:
					led_set_blue();
					break;
				case  LED_MODE_FLASH0:
				    duer_timer_start(s_led_timer,200);
					break;
				case  LED_MODE_FLASH1:
				    
					break;
			    case  LED_MODE_FLASH2:
					break;
				default:
					break;
		}
		return 0;
}

int  led_off(void)
{
		apa102_clear_strip(s_apa102led_dev);
		return 0;
}

int  led_on(void)
{
		return 0;
}


