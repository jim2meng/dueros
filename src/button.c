
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include  <wiringPi.h>
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

#include "button.h"

#define BUTTON_GPIO				(0)

static pthread_t s_button_threadID;

 void  isr_function(void)
 {
		//DUER_LOGI("isr_function %d\n",digitalRead(BUTTON_GPIO));
		return ;
}

int  duer_app_is_test_mode(void);

extern int  duer_recorder_test_start(int channel);

static void button_polling_thread()
{
	int old_state=0;
	int now_state=0;
	uint32_t  btn_press_time = duer_timestamp();
	uint32_t  btn_release_time = duer_timestamp();
	int  channel_id=1;
	
	 while(1)
	 {
			 waitForInterrupt(BUTTON_GPIO,100);	
			 if((now_state=digitalRead(BUTTON_GPIO))!=old_state)
			 {
					 	DUER_LOGI("%d %d\n",now_state,old_state);
					 	if(now_state==1) //release
					 	{
							    btn_release_time  =  duer_timestamp(NULL);
							    if(btn_release_time>btn_press_time+100)
							    {
										if(duer_app_is_test_mode())
										{
											    duer_recorder_test_start(channel_id);
											    duer_dcs_dialog_cancel();
												duer_media_tone_play("./resources/16.mp3");
												event_record_start();
												if(channel_id==1)
												{
														channel_id =2;
												}
												else
												{
														channel_id =1;
												}
										}
										else
										{
												duer_dcs_dialog_cancel();
												duer_media_tone_play("./resources/16.mp3");
												event_record_start();
										}	
								}
						}
						else  //press
						{
								btn_press_time = duer_timestamp();
						}
						
					 	old_state = now_state;
					 	
			}
	 }
}

int button_init(void)
{
		if(wiringPiSetup()!=0)
		{
				DUER_LOGE("wiringPiSetup fail\n");
				return -1;
		}
		
		if(pthread_create(&s_button_threadID, NULL, (void *)button_polling_thread, NULL)!=0)
		{
				DUER_LOGE("create button thread fail!\n");
				return -1;
		}
		 
		wiringPiISR(BUTTON_GPIO,INT_EDGE_BOTH,isr_function);
		
		return 0;
}




