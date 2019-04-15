
#ifndef  _LED_H_H___H___
#define  _LED_H_H___H___


#ifdef __cplusplus
extern "C"{
#endif

#define  LED_MODE_OFF									(0)
#define  LED_MODE_RED									(1)
#define  LED_MODE_GREEN								(2)
#define   LED_MODE_BLUE                                (3)
#define   LED_MODE_FLASH0							(4)
#define   LED_MODE_FLASH1							(5)
#define   LED_MODE_FLASH2							(6)

#define LED_MODE_RECORD_START			(LED_MODE_FLASH0)

#define LED_MODE_LISTEN_STOP				(LED_MODE_OFF)
	
int  led_init(void);

int led_set_mode(int mode);

int  led_off(void);

#ifdef __cplusplus
}
#endif

#endif
