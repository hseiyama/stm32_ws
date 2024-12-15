/**
  ******************************************************************************
  * @file           : lib.h
  * @brief          : ライブラリー共通定義
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LIB_H
#define __LIB_H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* タイマー情報 */
typedef struct _Timer {
	uint32_t u32_time;				/* タイマー開始時間						*/
	bool bl_state;					/* タイマー動作状態						*/
} Timer;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* lib_timer.c */
extern void taskTimerInit(void);											/* タイマー初期化処理					*/
extern void taskTimerUpdate(void);											/* タイマー更新処理						*/
extern void startTimer(Timer *pst_Timer);									/* タイマーを開始する					*/
extern void stopTimer(Timer *pst_Timer);									/* タイマーを停止する					*/
extern bool checkTimer(Timer *pst_Timer, uint32_t u32_WaitTime);			/* タイマーの満了を確認する				*/
extern bool isRunTimer(Timer *pst_Timer);									/* タイマーの動作状態を取得する			*/

/* lib_mem.s */
extern void mem_cpy32(uint32_t *dst, const uint32_t *src, size_t n);		/* memcpy(32bit版)						*/
extern void mem_cpy16(uint16_t *dst, const uint16_t *src, size_t n);		/* memcpy(16bit版)						*/
extern void mem_cpy08(uint8_t *dst, const uint8_t *src, size_t n);			/* memcpy(8bit版)						*/
extern void mem_set32(uint32_t *s, uint32_t c, size_t n);					/* memset(32bit版)						*/
extern void mem_set16(uint16_t *s, uint16_t c, size_t n);					/* memset(16bit版)						*/
extern void mem_set08(uint8_t *s, uint8_t c, size_t n);						/* memset(8bit版)						*/
extern int mem_cmp32(const uint32_t *s1, const uint32_t *s2, size_t n);		/* memcmp(32bit版)						*/
extern int mem_cmp16(const uint16_t *s1, const uint16_t *s2, size_t n);		/* memcmp(16bit版)						*/
extern int mem_cmp08(const uint8_t *s1, const uint8_t *s2, size_t n);		/* memcmp(8bit版)						*/

#endif /* __LIB_H */
