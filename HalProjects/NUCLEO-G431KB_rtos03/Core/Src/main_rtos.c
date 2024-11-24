/*
===============================================================================
 Name        : main_rtos.c
 Author      : 
 Version     :
 Copyright   : Copyright (C)
 Description : main definition
===============================================================================
*/

//#ifdef __USE_CMSIS
#include "main.h"
//#endif

// TODO: insert other include files here

// TODO: insert other definitions and declarations here


//
// Cortex-M3が例外発生時に自動的にPUSHするレジスタ類
//
struct ESTK_STRUC {
	unsigned int	r_r0;
	unsigned int	r_r1;
	unsigned int	r_r2;
	unsigned int	r_r3;
	unsigned int	r_r12;
							// r13はSP
	unsigned int	r_lr;	// r14 スレッド中のLR
	unsigned int	r_pc;	// r15
	unsigned int	r_xpsr;
};
typedef struct ESTK_STRUC	ESTK;

//
// タスクのスタック（タスクが非スケジュール状態の時のスタック上のデータ）
//
// r13はSPなのでスタックには積まないでTCBで管理する
//
struct TSTK_STRUC {
	unsigned int	r_r4;
	unsigned int	r_r5;
	unsigned int	r_r6;
	unsigned int	r_r7;
	unsigned int	r_r8;
	unsigned int	r_r9;
	unsigned int	r_r10;
	unsigned int	r_r11;

//------ここから先はCortex-M3が自動的に積む分------
	unsigned int	r_r0;
	unsigned int	r_r1;
	unsigned int	r_r2;
	unsigned int	r_r3;
	unsigned int	r_r12;
	unsigned int	r_lr;	// r14 スレッド中のLR
	unsigned int	r_pc;	// r15
	unsigned int	r_xpsr;
};
typedef struct TSTK_STRUC TSTK;

#define TSTKSIZE	((sizeof (struct TSTK_STRUC))/sizeof (int))	// 各タスクが非スケジュール時に使用するスタックサイズ
#define ESTKSIZE	((sizeof (struct ESTK_STRUC))/sizeof (int))	// Cortex-M3が例外発生時に自動的に使用するスタックサイズ
#define UPUSHSIZE	(TSTKSIZE-ESTKSIZE)

//
// タスクの状態コード
//
#define STATE_FREE	0x00
#define STATE_IDLE	0x01
#define STATE_READY	0x02
#define STATE_SLEEP	0x03

#define EOQ			0xff	// End Of Queue：キューの最後であることを示す。

#define MAX_TASKNUM	8


//
// TCB（Task Control Block)の定義
//
struct TCTRL_STRUC {
	unsigned char	link;
	unsigned char	state;
	unsigned char	msg_q;
	unsigned char	param0;
	TSTK			*sp;
	unsigned int	param1;
	unsigned int	param2;
};
typedef struct TCTRL_STRUC TCTRL;	// typedefすると見た目がちょっと格好良いかもっていう程度
TCTRL	tcb[MAX_TASKNUM];			// TCBをタスク数分確保

#define STKSIZE		64				// タスク用スタックサイズ（64ワード：256バイト）
unsigned int	stk_task[MAX_TASKNUM][STKSIZE];	// タスク用スタックエリア


//
// セマフォ
//
#define MAX_SEMA	8
unsigned char semadat[MAX_SEMA];

//
// メッセージブロック
//
// メッセージもキューで管理するほうが効率も格好も良いのだけども、
// 今回はメッセージブロックの総数が少ないので、実装の単純化を優先
// して、毎回全メッセージブロックをスキャンすることにした
//
#define MAX_MSGBLK	8
struct MSGBLK_STRUC {
	unsigned char	link;
	unsigned char	param_c;
	unsigned short	param_s;
	unsigned int	param_i;
};
typedef struct MSGBLK_STRUC MSGBLK;
MSGBLK msgblk[MAX_MSGBLK];


//
// UART送信
//
uint8_t UartWrite(uint8_t data)
{
	uint8_t RetCode = NG;
	__disable_irq();
	if ((USART2->ISR & USART_ISR_TXE_TXFNF) == USART_ISR_TXE_TXFNF) {
		USART2->TDR = data;
		RetCode = OK;
	}
	__enable_irq();
	return RetCode;
}

//
// UART受信
//
uint8_t UartRead(uint8_t *data)
{
	uint8_t RetCode = NG;
	__disable_irq();
	if ((USART2->ISR & USART_ISR_RXNE_RXFNE) == USART_ISR_RXNE_RXFNE) {
		*data = USART2->RDR;
		RetCode = OK;
	}
	__enable_irq();
	return RetCode;
}


//===============================================
//= TCB関係処理									=
//===============================================
unsigned char	q_pending[2];	// 処理待ちタスク（ON/OFF処理などで使用）
unsigned char	q_ready;		// 起動状態
unsigned char	q_sleep;		// スリープ状態（タイマ待ち）

unsigned char	task_start;
unsigned char	c_tasknum;		// 現在スケジュール中のタスク番号
TCTRL			*c_task;

//
// キューの最後に指定されたTCBをつなぐ
//
unsigned char tcbq_append(unsigned char *queue, unsigned char tcbnum)
{
	unsigned char ctasknum, ptasknum;
	if (tcbnum == EOQ)						// EOQだったらなにもしない
		return(EOQ);
	if ((ctasknum = *queue) == EOQ) {		// いま接続されている先は無い
		*queue = tcbnum;					// 与えられたTCBをつなぐ
	} else {
		do {								// 最後を探す
			ptasknum = ctasknum;			// PreviousTCBにCurrentTCB番号を保存
		} while((ctasknum = tcb[ptasknum].link) != EOQ);	// ptasknumが最後？
		tcb[ptasknum].link = tcbnum;		// 最後のTCBに指定されたTCBをつなぐ
	}
	tcb[tcbnum].link = EOQ;					// キューの終わりになるので、EOQ
	return(tcbnum);
}

//
// キューの先頭を取り出す
//
unsigned char tcbq_get(unsigned char *queue)
{
	unsigned char tcbnum;
	if ((tcbnum = *queue) != EOQ)			// キューの先頭を取り出す
		*queue = tcb[tcbnum].link;			// EOQでないなら、リンク先につなぎなおし
	return(tcbnum);							// もし、キューの先頭がEOQならEOQが返るだけ
}

//
// 指定されたTCBをキューから外す
//
unsigned char tcbq_remove(unsigned char *queue, unsigned char tcbnum)
{
	unsigned char ctasknum, ptasknum;
	ctasknum = *queue;
	if (tcbnum == EOQ)						// EOQだったらなにもしない
		return(EOQ);
	if (ctasknum == EOQ)					// キューに何もつながってない
		return(EOQ);						// ならEOQで終了
	if (ctasknum == tcbnum) {				// いきなり先頭
		*queue = tcb[tcbnum].link;			// つなぎ変えて
		return(ctasknum);					// 終了
	}
	do {									// マッチするものを探すループ
		ptasknum = ctasknum;				// PreviousにCurrentの値をコピー
		if ((ctasknum = tcb[ptasknum].link) == EOQ)	// リンク先がEOQだったら
			return(EOQ);					// EOQを返す
	} while(ctasknum != tcbnum);			// リンク先が一致するまでループ
	tcb[ptasknum].link = tcb[ctasknum].link;	// ctasknumのリンクを繋ぎ替え
	return(ctasknum);
}

//
// タスクON処理
//
unsigned char process_taskon(unsigned char tasknum)
{
	unsigned char c;
	c = tcb[tasknum].state;
	if ((c == STATE_FREE) || (c == STATE_IDLE)) {	// FreeかIdleのもの専用
		tcb[tasknum].state = STATE_READY;
		tcbq_append(&q_ready, tasknum);
		return(tasknum);
	}
	return(EOQ);
}

//
// タスクOFF処理
// 指定されたタスクがREADYまたはSLEEP状態であれば、外してIDLEステートにする
//
unsigned char process_taskoff(unsigned char tasknum)
{
	switch(tcb[tasknum].state) {
		case STATE_READY:
			tcbq_remove(&q_ready, tasknum);
			tcb[tasknum].state = STATE_IDLE;
			return(tasknum);
		case STATE_SLEEP:
			tcbq_remove(&q_sleep, tasknum);
			tcb[tasknum].state = STATE_IDLE;
			return(tasknum);
		default:
			return(EOQ);
	}
}

//
// スリープ処理
//
void process_sleep(void)
{
	unsigned char tasknum;
	tasknum = q_sleep;
	while(tasknum != EOQ) {					// キューの最後までスキャン
		if (tcb[tasknum].param1 == 0) {		// タイムアップした
			tcbq_remove(&q_sleep, tasknum);	// Sleep_Qから外す
			tcb[tasknum].state = STATE_READY;	// READY状態にする
			tcbq_append(&q_ready, tasknum);	// Ready_Qにつなぐ
		} else {
			tcb[tasknum].param1--;			// デクリメント
		}
		tasknum = tcb[tasknum].link;
	}
}


//===============================================
//= メッセージブロック関連処理					=
//===============================================
unsigned char q_msgblk;			// フリー状態

//
// システムタイマ割り込みの処理
//
unsigned char rq_timer;

unsigned int systick_count;
unsigned int sticks = 3;
void SysTick_Handler()
{
	if (systick_count)
		systick_count--;

	if (task_start) {
		if (sticks)
			sticks--;
		else {
			sticks = 3;
			rq_timer = 1;
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
	}
}


unsigned int pendsv_count;
// 
// PendSVのハンドラ（タスクスイッチングの実行）
//
// ところで、
// __attribute__ ((naked));
// は、
// indicate that the specified function does not need prologue/epilogue
// sequences generated by the compiler
// ということで、コンパイラによるレジスタ退避などを行わせないためのアトリビュート。
// コンパイラで使うスタック上のワーク領域なども自動確保するコード生成をしないので
// asm()の中で書いてやらなければならない。
// コンパイラがどれだけスタック領域を使うのかは実際__attribute__を外してコンパイル
// してみて確認するしかない。
//
unsigned char swstart = 0;
void schedule(void)
{
	unsigned char ctasknum, ntasknum;
	// デバッグ用にスイッチング回数をカウント
	pendsv_count++;
	// c_taskを切り替え（タスクスイッチング）
	if (swstart) {
		if (rq_timer) {
			process_sleep();
			rq_timer = 0;
		}
		if ((ctasknum = q_pending[0]) !=EOQ) {	// Pendingされているタスク制御処理がある
			switch(q_pending[1]) {
				case STATE_READY:	// Ready状態にしたい
					process_taskon(ctasknum);	// TASKON処理をする
					break;
				case STATE_IDLE:	// Idle状態にしたい
					process_taskoff(ctasknum);	// TASKOFF処理をする
					break;
				default:
					break;
			}
			q_pending[0] = EOQ;				// 処理したので、クリア
		}
		ctasknum = c_tasknum;				// 今までスケジュールしていたタスク番号を退避
		ntasknum = tcbq_get(&q_ready);		// Ready_Qから取り出す
		if (ntasknum == EOQ) {				// Ready_Qが無い
			if (c_tasknum == EOQ)			// スケジュールしていたものもない！？？
				c_tasknum = 0;				// ありえないけど、0にしておく
		} else {							// Ready_Qにつながっていた
			c_tasknum = ntasknum;			// Ready_Qから取り出したのをスケジュール状態にする
			switch(tcb[ctasknum].state) {
				case STATE_READY: tcbq_append(&q_ready, ctasknum); break;
				case STATE_SLEEP: tcbq_append(&q_sleep, ctasknum); break;
				default: break;
			}
		}
		for (ctasknum = 0; ctasknum < MAX_TASKNUM; ctasknum++) {
			if ((tcb[ctasknum].state == STATE_IDLE) && (tcb[ctasknum].msg_q != EOQ))
				process_taskon(ctasknum);
		}
	} else {
		swstart = 1;
	}
	c_task = &tcb[c_tasknum];
}

void PendSV_Handler(void) __attribute__ ((naked));
void PendSV_Handler()
{
	// 前半部分では、
	// ・自動退避されない汎用レジスタをR12でstmdb(Dec. Before)を使ってPSP上に退避
	// ・c_task（現在実行中のタスクのTCBをさしている）にR12を退避
	// を実行
	asm(									// R12をワーク用スタックとして利用
	"	mrs		r12,psp					\n"	// R12 = PSP;
	"	stmdb	r12!,{r4-r11}			\n"	// 自動退避されないR4～R11を退避
	"	movw	r2,#:lower16:c_task		\n"	// R2 = &c_task;
	"	movt	r2,#:upper16:c_task		\n"
	"	ldr		r0,[r2,#0]				\n"
	"	str		r12,[r0,#4]				\n"	// *(ctask->sp) = R12;
	);

	// 次にスケジュールするタスクの選択
	asm(
	"	push	{lr}					\n"
	"	bl		schedule				\n"
	"	pop		{lr}					\n"
	);

	// 後半部分では
	// 新しいc_taskの指す先（次に動かすタスクのTCB）からレジスタを復帰
	// 今度は
	// ・R12を取り出し
	// ・汎用レジスタを復旧（ldmia(Inc. After)を利用）
	// 元に戻る
	asm(
	"	movw	r2,#:lower16:c_task		\n"	// R2 = &c_task;
	"	movt	r2,#:upper16:c_task		\n"
	"	ldr		r0,[r2,#0]				\n"
	"	ldr		r12,[r0,#4]				\n"	// R12 = *(c_task->sp);
	"	ldmia	r12!,{r4-r11}			\n"	// 退避したR4～R11を復帰
	"	msr		psp,r12					\n"	// PSP = R12;
	"	bx		lr						\n"	// return;
	);
}

unsigned int svcop;
unsigned int svcparam[2];
void SVC_Handler(void) __attribute__ ((naked));
void SVC_Handler()
{
	asm(
	"	movw	r2,#:lower16:svcparam	\n"	// R2 = &svcparam;
	"	movt	r2,#:upper16:svcparam	\n"
	"	str		r0,[r2,#0]				\n"	// svcparam[0] = R0;
	"	str		r1,[r2,#4]				\n"	// svcparam[1] = R1;
	"	mov		r0,lr					\n"	// if ((LR & 0x04) != 0) {
	"	ands	r0,#4					\n"	//					// LRのビット4が'0'ならハンドラモードでSVC
	"	beq		.L0000					\n"	//					// '1'ならスレッドモードでSVC
	"	mrs		r1,psp					\n"	//   R1 = PSP;		// プロセススタックをコピー
	"	b		.L0001					\n"
	".L0000:							\n"	// } else {
	"	mrs		r1,msp					\n"	//   R1 = MSP;		// メインスタックをコピー
	".L0001:							\n"	// }
	"	ldr		r2,[r1,#24]				\n"	// R2 = R1->PC;
	"	ldrh	r0,[r2,#-2]				\n"	// R0 = *(R2-2);	// SVC(SWI)命令の下位バイトが引数部分
	"	movw	r2,#:lower16:svcop		\n"	// R2 = &svcop;
	"	movt	r2,#:upper16:svcop		\n"
	"	str		r0,[r2,#0]				\n"	// svcop = R0;		// svcop変数にコピー

	"	push	{r7}					\n"	// PUSH R7			// C言語でフレームポインタにR7を使っているため
	"	sub		sp,sp,#8				\n"	// SP -= 8;			// (4バイト×2個分）：C言語側で２ワード分使っていたため
	"	mov		r7,sp					\n"	// R7 = SP;
	);

	switch(svcop & 0xff) {							// SVCの引数（リクエストコード）を取得
		case 0x00:	// NULL（再スケジューリングするだけ）
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;	// PendSVを発生させてスケジューリング
			break;
		case 0x01:	// LEDOFF
			GPIOB->BRR = GPIO_BRR_BR8_Msk;
			break;
		case 0x02:	// LEDON
			GPIOB->BSRR = GPIO_BSRR_BS8_Msk;
			break;
		case 0x10:	// SLEEP
			tcb[c_tasknum].param1 = svcparam[0];	// パラメータを積んで
			tcb[c_tasknum].state = STATE_SLEEP;		// スリープ状態にしておく
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;	// PendSVを発生させてスケジューリング
			break;
		case 0x11:	// TASKON
			q_pending[0] = svcparam[0];				// タスク番号をPendingにセット
			q_pending[1] = STATE_READY;				// READY状態に遷移させたい
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;	// PendSVを発生させてスケジューリング
			break;
		case 0x12:	// TASKOFF
			q_pending[0] = svcparam[0];				// タスク番号をPendingにセット
			q_pending[1] = STATE_IDLE;				// IDLE状態に遷移させたい
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;	// PendSVを発生させてスケジューリング
			break;
		case 0x13:	// SEMAGET(data#)
			asm(
			"	movw	r2,#:lower16:svcparam	\n"	// R2 = &svcparam;
			"	movt	r2,#:upper16:svcparam	\n"
			"	ldr		r0,[r2,#0]				\n"	// R0 = svcparam[0];(DATA#)
			"	movw	r2,#:lower16:semadat	\n"	// R2 = &semadat;
			"	movt	r2,#:upper16:semadat	\n"
			"	add		r2,r0					\n"
			"	mov		r0,#0					\n"
			"	ldrb	r0,[r2,#0]				\n"	// R0 = semadat[DATA#];
			"	str		r0,[r1,#0]				\n"	// R1->R0 = R0;(return R0)
			"	cmp		r0,#0					\n"	// if (R0 == 0) {
			"	bne		.L0100					\n"
			"	mov		r0,#1					\n"	//   R0 = 1;
			"	str		r0,[r2,#0]				\n"	//   semadat[DATA#] = R0;
			".L0100:							\n"	// }
			);
			break;
		case 0x14:	// CLRSEMA
			semadat[svcparam[0]] = 0;
			break;
		case 0x15:	// MSGBLKGET
			asm(
			"	movw	r3,#:lower16:q_msgblk	\n"	// R3 = &q_msgblk;
			"	movt	r3,#:upper16:q_msgblk	\n"
			"	ldrb	r0,[r3,#0]				\n"	// R0 = q_msgblk;
			"	cmp		r0,#0xff				\n"	// if (R0 != EOQ) {
			"	beq		.L1500					\n"
			"	movw	r2,#:lower16:msgblk		\n"	//   R2 = &msgblk;
			"	movt	r2,#:upper16:msgblk		\n"
			"	ldrb	r2,[r2,r0, lsl #3]		\n"	//   R2 = msgblk[R0].link;
			"	strb	r2,[r3,#0]				\n"	//   q_msgblk = R2;
			"	movw	r2,#:lower16:msgblk		\n"	//   R2 = &msgblk;
			"	movt	r2,#:upper16:msgblk		\n"
			"	mov		r3,#0xff				\n"
			"	strb	r3,[r2,r0, lsl #3]		\n" //   msgblk[R0].link = 0xff;
			".L1500:							\n"	// }
			"	str		r0,[r1,#0]				\n"	// return(R0);
			);
			break;
		case 0x16:	// MSGBLKFREE(data#)
			asm(
			"	movw	r2,#:lower16:svcparam	\n"	// R2 = &svcparam;
			"	movt	r2,#:upper16:svcparam	\n"
			"	ldrb	r1,[r2,#0]				\n"	// R1 = svcparam[0];(DATA#)
			"	movw	r2,#:lower16:msgblk		\n"	// R2 = &msgblk;
			"	movt	r2,#:upper16:msgblk		\n"
			"	movw	r3,#:lower16:q_msgblk	\n"	// R3 = &q_msgblk;
			"	movt	r3,#:upper16:q_msgblk	\n"
			"	ldrb	r0,[r3,#0]				\n"	// R0 = q_msgblk;
			"	strb	r1,[r3,#0]				\n"	// q_msgblk = DATA#;
			"	strb	r0,[r2,r1, lsl #3]		\n"	// msgblk[DATA#].link = R0;
			);
			break;
		case 0x17:	// MSGBLKSEND(task#, msgblk#)
			asm(
			"	movw	r3,#:lower16:svcparam	\n"	// R3 = &svcparam;
			"	movt	r3,#:upper16:svcparam	\n"
			"	ldr		r0,[r3,#0]				\n"	// R0 = svcparam[0];(TASK#)
			"	mov		r0,r0, lsl #4			\n"
			"	ldr		r1,[r3,#4]				\n"	// R1 = svcparam[1];(MSGBLK#)
			"	movw	r3,#:lower16:tcb		\n"	// R3 = &tcb;
			"	movt	r3,#:upper16:tcb		\n"
			"	add		r0,#2					\n"
			"	ldrb	r2,[r3,r0]				\n"	// R2 = tcb[TASK#].msg_q;
			"	cmp		r2,#0xff				\n"	// if (R2 == EOQ) {
			"	bne		.L1700					\n"
			"	strb	r1,[r3,r0]				\n"	//   tcb[TASK#].msg_q = MSGBLK#;
			"	b		.L1703					\n"	// } else {
			".L1700:							\n"
			"	movw	r3,#:lower16:msgblk		\n"	//   R3 = &msgblk;
			"	movt	r3,#:upper16:msgblk		\n"
			".L1701:							\n"	//   while (1) {
			"	ldrb	r0,[r3,r2, lsl #3]		\n"	//     R0 = msgblk[R2].link
			"	cmp		r0,#0xff				\n"	//     if (R0 == EOQ)
			"	beq		.L1702					\n"	//       break;
			"	mov		r2,r0					\n"	//     R2 = R0;
			"	b		.L1701					\n"	//   }
			".L1702:							\n"
			"	strb	r1,[r3,r2, lsl #3]		\n"	//   msgblk[R2].link = MSGBLK#;
			".L1703:							\n"	// }
			"	movw	r3,#:lower16:msgblk		\n"	// R3 = &msgblk;
			"	movt	r3,#:upper16:msgblk		\n"
			"	mov		r0,#0xff				\n"
			"	strb	r0,[r3,r1, lsl #3]		\n"	// msgblk[R1].link = EOQ;
			);
			break;
		case 0x18:	// MSGBLKRCV
			asm(
			"	movw	r2,#:lower16:c_tasknum	\n"	// R2 = &c_tasknum;
			"	movt	r2,#:upper16:c_tasknum	\n"
			"	ldrb	r2,[r2,#0]				\n"	// R2 = c_tasknum;
			"	mov		r2,r2, lsl #4			\n"
			"	add		r2,#2					\n"
			"	movw	r3,#:lower16:tcb		\n"	// R3 = &tcb;
			"	movt	r3,#:upper16:tcb		\n"
			"	add		r3,r2					\n"	// R3 = &(tcb[c_tasknum].msg_q);
			"	ldrb	r0,[r3,#0]				\n"	// R0 = tcb[c_tasknum].msg_q;
			"	cmp		r0,#0xff				\n"	// if (R0 != EOQ) {
			"	beq		.L1800					\n"
			"	movw	r2,#:lower16:msgblk		\n"	//   R2 = &msgblk;
			"	movt	r2,#:upper16:msgblk		\n"
			"	ldrb	r2,[r2,r0, lsl #3]		\n"	//   R2 = &msgblk[R0];
			"	strb	r2,[r3,#0]				\n"	//   tcb[c_tasknum].msg_q = msgblk[R0].link;
			".L1800:							\n"	// }
			"	str		r0,[r1,#0]				\n"	// return(R0);
			);
			break;
		case 0x19:	// TASKIDGET
			asm(
			"	movw	r0,#:lower16:c_tasknum	\n"	// R0 = &c_tasknum;
			"	movt	r0,#:upper16:c_tasknum	\n"
			"	ldrb	r0,[r0,#0]				\n"	// R0 = c_tasknum;
			"	str		r0,[r1,#0]				\n"	// return(R0);
			);
			break;
		case 0xf0:	// CHG_IDLE
			tcb[c_tasknum].state = STATE_IDLE;
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;	// PendSVを発生させてスケジューリング
			break;
		case 0xff:	// CHG_UNPRIVILEGE
			asm(
			"	movw	r2,#:lower16:c_task		\n"	// R2 = &c_task;
			"	movt	r2,#:upper16:c_task		\n"
			"	ldr		r0,[r2,#0]				\n"
			"	ldr		r2,[r0,#4]				\n"
			"	msr		psp,r2					\n"	// PSP = *(c_task->sp);
			"	orr		lr,lr,#4				\n"	// LR |= 0x04;
													// スレッドモードに移行
													// 1001:msp使用(プロセス） 1101:psp使用（スレッド）
													// なので、セットするとスレッドモードになる
			);
			task_start = 1;							// 以降はタスクスイッチング実行
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;	// PendSVを発生させる
			break;
		default:
			break;
	}
	asm(
	"	add		sp,sp,#8				\n"	// SP += 8
	"	pop		{r7}					\n"	// POP R7
	"	bx		lr						\n"	// return;
	);
}


//=======================================
//= ここからはタスク					=
//=======================================

//===============================
//=								=
//= SVCの定義					=
//=								=
//===============================

#define SYSCALL_NULL	asm("svc #0x00\n\t")
#define SYSCALL_LEDOFF	asm("svc #0x01\n\t")
#define SYSCALL_LEDON	asm("svc #0x02\n\t")

#define SYSCALL_IDLE	asm("svc #0xf0\n\t")
#define SYSCALL_CHG_UNPRIVILEGE	asm("svc #0xff\n\t")

#define SYSCALL_SLEEP(x)	{register int p0 asm("r0"); p0=x; asm("svc #0x10"::"r"(p0));}
#define SYSCALL_TASKON(x)	{register int p0 asm("r0"); p0=x; asm("svc #0x11"::"r"(p0));}
#define SYSCALL_TASKOFF(x)	{register int p0 asm("r0"); p0=x; asm("svc #0x12"::"r"(p0));}
#define SYSCALL_SEMAGET(x)	{register int p0 asm("r0"); p0=x; asm("svc #0x13"::"r"(p0));}
#define SYSCALL_SEMACLR(x)	{register int p0 asm("r0"); p0=x; asm("svc #0x14"::"r"(p0));}
#define SYSCALL_MSGBLKGET	asm("svc #0x15\n\t")
#define SYSCALL_MSGBLKFREE(x)	{register int p0 asm("r0"); p0=x; asm("svc #0x16"::"r"(p0));}
#define SYSCALL_MSGBLKSEND(tcb,msgblk)	{register int p0 asm("r0"); register int p1 asm("r1"); p0=tcb; p1=msgblk;  asm("svc #0x17"::"r"(p0),"r"(p1));}
#define SYSCALL_MSGBLKRCV	asm("svc #0x18\n\t")
#define SYSCALL_TASKIDGET	asm("svc #0x19\n\t")


//===============================
//=								=
//= SVCインターフェースAPI		=
//=								=
//===============================

//
// NULL:スケジューラを呼び出し、次のタスクを起動する
//
void SVC_NULL(void)
{
	SYSCALL_NULL;
}

//
// LEDOFF：ボード上のLEDを消灯する
//
void SVC_LEDOFF(void)
{
	SYSCALL_LEDOFF;
}

//
// LEDON：ボード上のLEDを点灯する
//
void SVC_LEDON(void)
{
	SYSCALL_LEDON;
}

//
// IDLE:IDLEステートに移行する（メッセージ待ちなどで利用）
//
void SVC_IDLE(void)
{
	SYSCALL_IDLE;
}

//
// スレッドモードに移行（main()からMicroMultiを起動する時に利用するだけ）
//
void SVC_CHG_UNPRIVILEGE(void)
{
	SYSCALL_CHG_UNPRIVILEGE;
}

//
// SLEEP：指定した回数分システムタイマ割り込みがくるまでスリープ
//
void SVC_SLEEP(unsigned int times)
{
	SYSCALL_SLEEP(times);
}

//
// TASKON：指定したタスク番号のタスクを起動
//
void SVC_TASKON(unsigned int tasknum)
{
	SYSCALL_TASKON(tasknum);
}

//
// TASKOFF：指定したタスク番号のタスクを停止
//
void SVC_TASKOFF(unsigned int tasknum)
{
	SYSCALL_TASKOFF(tasknum);
}

//
// SEMAGET：指定した番号のセマフォ取得要求
// セマフォが取れれば0、取れなければ0以外が返る
//
unsigned char SVC_SEMAGET(unsigned char d)
{
	register unsigned int retval asm("r0");	// retvalをR0レジスタに割付け
	SYSCALL_SEMAGET(d);
	return(retval);
}

//
// セマフォが取れるまで待つ
//
void SVC_SEMAGET_W(unsigned char d)
{
	while(SVC_SEMAGET(d))
		;
}

//
// SEMACLR：指定した番号のセマフォをクリア（返却）
//
void SVC_SEMACLR(unsigned char d)
{
	SYSCALL_SEMACLR(d);
}

//
// MSGBLKGET:フリーなメッセージブロックを取り出す
//
unsigned char SVC_MSGBLKGET()
{
	register unsigned char c asm("r0");
	SYSCALL_MSGBLKGET;
	return(c);
}

//
// メッセージブロックが取得できるまでウェイトするタイプ
//
unsigned char SVC_MSGBLKGET_W()
{
	unsigned char c;
	while((c = SVC_MSGBLKGET()) == EOQ)
		;
	return(c);
}

//
// MSGBLKFREE：指定したメッセージブロックを返却
//
void SVC_MSGBLKFREE(unsigned char d)
{
	SYSCALL_MSGBLKFREE(d);
}

//
// MSGBLKSEND：指定したタスクにメッセージブロックを送信
//
void SVC_MSGBLKSEND(unsigned char tcbnum, unsigned char msgblk)
{
	SYSCALL_MSGBLKSEND(tcbnum, msgblk);
}

//
// MSGBLKRCV：メッセージ受信
// メッセージがあればメッセージ番号、無ければEOQ（0xff）が返る
//
unsigned char SVC_MSGBLKRCV()
{
	register unsigned char c asm("r0");
	SYSCALL_MSGBLKRCV;
	return(c);
}

//
// TASKIDGET：タスクID取得
//
unsigned char SVC_TASKIDGET()
{
	register unsigned char c asm("r0");
	SYSCALL_TASKIDGET;
	return(c);
}

//===============================
//=								=
//= タスク関数					=
//=								=
//===============================

unsigned char dbgdata[3];
void th_zero()
{
	uint8_t RcvData;
	dbgdata[0] = SVC_TASKIDGET();
	while(1) {
		if (UartRead(&RcvData) == OK) {
			UartWrite(RcvData);
		}
		SVC_NULL();
	}
}

void th_ledoff()
{
	while(1) {
		SVC_LEDOFF();
	}
}

void th_blink()
{
	unsigned char mblk;
	dbgdata[1] = SVC_TASKIDGET();
	SVC_TASKON(2);
	while(1) {
		UartWrite('B');
		mblk = SVC_MSGBLKGET_W();
		msgblk[mblk].param_c = 1;
		SVC_MSGBLKSEND(2, mblk);
		SVC_SLEEP(200);
		mblk = SVC_MSGBLKGET_W();
		msgblk[mblk].param_c = 0;
		SVC_MSGBLKSEND(2, mblk);
		SVC_SLEEP(200);
		mblk = SVC_MSGBLKGET_W();
		msgblk[mblk].param_c = 2;
		SVC_MSGBLKSEND(2, mblk);
		SVC_SLEEP(200);
	}
}

void th_ledon()
{
	unsigned int i,j,k;
	unsigned char mblk;
	dbgdata[2] = SVC_TASKIDGET();
	while(1) {
		UartWrite('C');
		SVC_IDLE();
		do {
			mblk = SVC_MSGBLKRCV();
		} while(mblk == EOQ);
		switch(msgblk[mblk].param_c) {
			case 0x00:
				SVC_LEDOFF();
				break;
			case 0x01:
				SVC_LEDON();
				break;
			default:
				for (k=0; k<10; k++) {
					SVC_LEDON();
					for (i=0; i<3; i++)
						for (j=0; j<0xfffe; j++)
							;
					SVC_LEDOFF();
					for (i=0; i<3; i++)
						for (j=0; j<0xfffe; j++)
							;
				}
				break;
		}

		SVC_MSGBLKFREE(mblk);
	}
}


//===============================
//=								=
//= スタートアップと初期化関数	=
//=								=
//===============================

//
// メッセージブロックの初期化
//
//  全ブロックともフリー状態にするだけ
void init_msgblk()
{
	unsigned char c;
	q_msgblk = 0;
	for (c=0; c<MAX_MSGBLK; c++) {
		msgblk[c].link = c+1;
		msgblk[c].param_c = 0;
		msgblk[c].param_s = 0;
		msgblk[c].param_i = 0;
	}
	msgblk[MAX_MSGBLK-1].link = EOQ;
}

//
// セマフォデータの初期化
//
void init_semadat()
{
	unsigned char c;
	for (c=0; c<MAX_SEMA; c++)
		semadat[c] = 0;
}

//
// TCBの初期化とタスク登録
//
// XPSRのビット24は'1'にしないとハードエラーになるので注意
//
// スタックデータ作成時直接キャストしてアクセスすると
// コード生成を間違うようなので、一旦unsigned intに
// キャストして代入しなおした。
//
unsigned int *p;
TSTK	*p_stk;

void init_tcb()
{
	unsigned char tasknum;
	for (tasknum = 0; tasknum <MAX_TASKNUM; tasknum++)  {
		tcb[tasknum].state = STATE_FREE;
		tcb[tasknum].link = EOQ;
		tcb[tasknum].msg_q = EOQ;
	}
}

void regist_tcb(unsigned char tasknum, void(*task)(void))
{
	p = stk_task[tasknum];
	p += (STKSIZE-TSTKSIZE);
	p_stk = (TSTK*)(p);
	p_stk->r_r0 = 0x00;
	p_stk->r_r1 = 0x01;
	p_stk->r_r2 = 0x02;
	p_stk->r_r3 = 0x03;
	p_stk->r_r12 = 0x12;
	p_stk->r_lr = 0x00;
	p_stk->r_pc = (unsigned int)(task);
	p_stk->r_xpsr = 0x01000000;
	tcb[tasknum].sp = (TSTK*)(p);
}

//
// ここからメインルーチン
// 諸々の初期化をしてから、マルチタスクモードに移行
//
int main_rtos(void)
{
	// TODO: insert code here
	unsigned int i;
	init_tcb();
	init_msgblk();
	init_semadat();

	rq_timer = 0;
	task_start = 0;
	c_tasknum = 0;
	c_task = &tcb[c_tasknum];
//	GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODE8_Msk) | GPIO_MODER_MODE8_0;
//	GPIOB->BRR = GPIO_BRR_BR8_Msk;

	pendsv_count = 0;
	systick_count = 100;
//	SysTick_Config(SystemCoreClock/100);	// 1/100秒（=10ms）ごとにSysTick割り込み
//	SysTick_Config(SystemCoreClock/10);		// 1/10秒（=100ms）ごとにSysTick割り込み
	NVIC_SetPriority(SVCall_IRQn, 0x80);	// SVCの優先度は中ほど
	NVIC_SetPriority(SysTick_IRQn, 0xc0);	// SysTickの優先度はSVCより低く
	NVIC_SetPriority(PendSV_IRQn, 0xff);	// PendSVの優先度を最低にしておく
	NVIC_EnableIRQ(SysTick_IRQn);

	// Enter an infinite loop, just incrementing a counter
	for (i=0; i<10; i++) {
		while(systick_count)
			;
//		systick_count = 20;
		systick_count = 100;
		SVC_LEDON();
		while(systick_count)
			;
//		systick_count = 20;
		systick_count = 100;
		SVC_LEDOFF();
	}

	// この段階ではまだ特権モードであり、MSPが使われている
	// PSPプロセススタックは非特権モードで動いている状態
	// タスク#0（main()から直接切り替わって動くタスク）のスタック
	// については、ユーザが積む分だけ削っておく
	// CHG_UNPRIVILEGEでSVC=>PendSVの時にPSPに積みなおすことで
	// つじつまが合う
	q_ready = EOQ;
	q_sleep = EOQ;
	q_pending[0] = EOQ;

	regist_tcb(0,th_zero);
	tcb[0].sp = (TSTK *)(((unsigned int *)tcb[0].sp)+UPUSHSIZE);
	tcb[0].state = STATE_READY;

	regist_tcb(1,th_blink);
	tcb[1].state = STATE_READY;

	regist_tcb(2,th_ledon);
	tcb[2].state = STATE_IDLE;

	// Ready_Qにつないでおく
	// 最初は#0が動くことにしたので、#0はいきなりCurrentTASKになるため、
	// Ready_Qにつなぐのは0以外
	tcbq_append(&q_ready, 1);
//	tcbq_append(&q_ready, 2);

	tcb[3].state = STATE_FREE;
	tcb[3].link = EOQ;

	SVC_CHG_UNPRIVILEGE();	// スレッド#0に移行する

	while(1);		// 戻ってくることは無いけど、念のため
}

