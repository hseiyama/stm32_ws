/**
  ******************************************************************************
  * @file           : lib_tim.c
  * @brief          : タイマー操作ライブラリー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define SYS_TIME_MAX		(0xFFFFFFFF)			/* システムタイマー最大値		*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile static uint32_t u32s_SystemTimer;			/* システムタイマー(1ms)		*/

/* Private function prototypes -----------------------------------------------*/
static uint32_t getTimerDiff(Timer *pst_Timer, uint32_t u32_WaitTime);		/* 満了までの待ち時間[ms]を取得する		*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief タイマー初期化処理
  * @param None
  * @retval None
  */
void taskTimerInit(void)
{
	u32s_SystemTimer = 0;
}

/**
  * @brief タイマー更新処理
  * @param None
  * @retval None
  */
void taskTimerUpdate(void)
{
	u32s_SystemTimer += SYS_CYCLE_TIME;
}

/**
  * @brief タイマーを開始する
  * @param pst_Timer タイマー情報(構造体)のポインタ
  * @retval None
  */
void startTimer(Timer *pst_Timer)
{
	pst_Timer->u32_time = u32s_SystemTimer;
	pst_Timer->bl_state = true;
}

/**
  * @brief タイマーを停止する
  * @param pst_Timer タイマー情報(構造体)のポインタを指定する
  * @retval None
  */
void stopTimer(Timer *pst_Timer)
{
	pst_Timer->u32_time = SYS_TIME_MAX;
	pst_Timer->bl_state = false;
}

/**
  * @brief タイマーの満了を確認する
  * @param pst_Timer タイマー情報(構造体)のポインタ
  * @param u32_WaitTime 待ち時間
  * @retval bool
  */
bool checkTimer(Timer *pst_Timer, uint32_t u32_WaitTime)
{
	uint32_t u32_TimeDiff;
	bool bl_RetCode;

	/* タイマーが開始している場合 */
	if (pst_Timer->bl_state) {
		u32_TimeDiff = getTimerDiff(pst_Timer, u32_WaitTime);
		/* 待ち時間が経過している場合は、trueを設定 */
		bl_RetCode = (u32_TimeDiff > 0) ? false : true;
	}
	/* タイマーが停止している場合は、falseを設定 */
	else {
		bl_RetCode = false;
	}

	return bl_RetCode;
}

/**
  * @brief タイマーの動作状態を取得する
  * @param pst_Timer タイマー情報(構造体)のポインタ
  * @retval bool
  */
bool isRunTimer(Timer *pst_Timer)
{
	return pst_Timer->bl_state;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief 満了までの待ち時間[ms]を取得する
  * @param pst_Timer タイマー情報(構造体)のポインタ
  * @param u32_WaitTime 待ち時間
  * @retval None
  */
static uint32_t getTimerDiff(Timer *pst_Timer, uint32_t u32_WaitTime)
{
	uint32_t u32_SystemTime;
	uint32_t u32_PassTime;
	uint32_t u32_RetTime;

	/* タイマーが開始している場合 */
	if (pst_Timer->bl_state) {
		/* システムタイマーをラッチする */
		u32_SystemTime = u32s_SystemTimer;
		/* 経過時間を算出する */
		if (u32_SystemTime >= pst_Timer->u32_time) {
			u32_PassTime = u32_SystemTime - pst_Timer->u32_time;
		}
		else {
			u32_PassTime = SYS_TIME_MAX - (pst_Timer->u32_time - u32_SystemTime);
		}
		/* 待ち時間との差を算出する（経過後は、0を設定）*/
		if (u32_WaitTime > u32_PassTime) {
			u32_RetTime = u32_WaitTime - u32_PassTime;
		}
		else {
			u32_RetTime = 0;
		}
	}
	/* タイマーが停止している場合は、システムタイマー最大値を設定 */
	else {
		u32_RetTime = SYS_TIME_MAX;
	}

	return u32_RetTime;
}
