/**
  ******************************************************************************
  * @file           : main_app.c
  * @brief          : MAINアプリケーション
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TIME_1S				(1000)					/* 1秒判定時間[ms]			*/
#define SND_BUFF_SIZE		(64)					/* 送信バッファサイズ		*/
#define RCV_BUFF_SIZE		(64)					/* 受信バッファサイズ		*/

/* SYNC命令 */
#define SYNC_LED_BRINK_A	(0x01)					/* LED点滅A(全Core点滅)		*/
#define SYNC_LED_BRINK_B	(0x02)					/* LED点滅B(時間差点滅)		*/
#define SYNC_LED_BRINK_C	(0x03)					/* LED点滅C(各Core反転)		*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Timer sts_Timer1s;							/* 1秒タイマー				*/
static uint8_t u8s_SndData[SND_BUFF_SIZE];			/* 送信データ				*/
static uint8_t u8s_RcvData[RCV_BUFF_SIZE];			/* 受信データ				*/
static uint16_t u16_RcvDataSize;					/* 受信データサイズ			*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初期化関数
  * @param  None
  * @retval None
  */
void setup(void)
{
	mem_set08(&u8s_SndData[0], 0x00, SND_BUFF_SIZE);
	mem_set08(&u8s_RcvData[0], 0x00, RCV_BUFF_SIZE);
	u16_RcvDataSize = 0;

	/* タイマーを開始する */
	startTimer(&sts_Timer1s);

	/* プログラム開始メッセージを表示する */
	uartEchoStrln("Start UART/GPIO sample!!");
}

/**
  * @brief  周期処理関数
  * @param  None
  * @retval None
  */
void loop(void)
{
	/* SYNC受信データを取得する */
	u16_RcvDataSize = syncGetRxData(&u8s_RcvData[0], RCV_BUFF_SIZE);
	/* SYNC受信データが存在する場合 */
	if (u16_RcvDataSize > 0) {
		/* UART送信データを登録する */
		uartSetTxData(&u8s_RcvData[0], u16_RcvDataSize);
	}

	/* UART受信データを取得する */
	u16_RcvDataSize = uartGetRxData(&u8s_RcvData[0], RCV_BUFF_SIZE);
	/* UART受信データが存在する場合 */
	if (u16_RcvDataSize > 0) {
		/* UART送信データを登録する */
		uartSetTxData(&u8s_RcvData[0], u16_RcvDataSize);
		/* LED点滅の同期制御 */
		switch (u8s_RcvData[0]) {
		/* LED点滅A(全Core点滅) */
		case 0x01: /* Ctrl + A */
			/* SYNC送信データを登録する */
			u8s_SndData[0] = SYNC_LED_BRINK_A;
			syncSetTxData(&u8s_SndData[0], 1);
			/* ユーザーLEDを点灯する */
			HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, HIGH);
			/* タイマーを再開する */
			startTimer(&sts_Timer1s);
			break;
		/* LED点滅B(時間差点滅) */
		case 0x02: /* Ctrl + B */
			/* SYNC送信データを登録する */
			u8s_SndData[0] = SYNC_LED_BRINK_B;
			syncSetTxData(&u8s_SndData[0], 1);
			/* ユーザーLEDを点灯する */
			HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, HIGH);
			/* タイマーを再開する */
			startTimer(&sts_Timer1s);
			break;
		/* LED点滅C(各Core反転) */
		case 0x03: /* Ctrl + C */
			/* SYNC送信データを登録する */
			u8s_SndData[0] = SYNC_LED_BRINK_C;
			syncSetTxData(&u8s_SndData[0], 1);
			/* ユーザーLEDを消灯する */
			HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, LOW);
			/* タイマーを再開する */
			startTimer(&sts_Timer1s);
			break;
		default:
			break;
		}
	}

	/* 1秒判定時間が満了した場合 */
	if (checkTimer(&sts_Timer1s, TIME_1S)) {
		/* ユーザーLEDを反転出力する */
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		/* 文字を出力する */
		uartEchoStr(".");

		/* タイマーを再開する */
		startTimer(&sts_Timer1s);
	}
}

/* Private functions ---------------------------------------------------------*/

