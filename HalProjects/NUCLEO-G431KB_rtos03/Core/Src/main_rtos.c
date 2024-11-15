/*
===============================================================================
 Name        : main.c
 Author      : 
 Version     :
 Copyright   : Copyright (C)
 Description : main definition
===============================================================================
*/

//#ifdef __USE_CMSIS
#include "LPC13xx.h"
//#endif

// TODO: insert other include files here

// TODO: insert other definitions and declarations here



//
// Cortex-M3����O�������Ɏ����I��PUSH���郌�W�X�^��
//
struct ESTK_STRUC {
	unsigned int	r_r0;
	unsigned int	r_r1;
	unsigned int	r_r2;
	unsigned int	r_r3;
	unsigned int	r_r12;
				// r13��SP
	unsigned int	r_lr;	// r14	�X���b�h����LR
	unsigned int	r_pc;	// r15
	unsigned int	r_xpsr;
};
typedef struct ESTK_STRUC	ESTK;

//
// �^�X�N�̃X�^�b�N�i�^�X�N����X�P�W���[����Ԃ̎��̃X�^�b�N��̃f�[�^�j
//
// r13��SP�Ȃ̂ŃX�^�b�N�ɂ͐ς܂Ȃ���TCB�ŊǗ�����
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

//------����������Cortex-M3�������I�ɐςޕ�------
	unsigned int	r_r0;
	unsigned int	r_r1;
	unsigned int	r_r2;
	unsigned int	r_r3;
	unsigned int	r_r12;
	unsigned int	r_lr;	// r14	�X���b�h����LR
	unsigned int	r_pc;	// r15
	unsigned int	r_xpsr;
};
typedef struct TSTK_STRUC TSTK;

#define	TSTKSIZE	((sizeof (struct TSTK_STRUC))/sizeof (int))	// �e�^�X�N����X�P�W���[�����Ɏg�p����X�^�b�N�T�C�Y
#define	ESTKSIZE	((sizeof (struct ESTK_STRUC))/sizeof (int))	// Cortex-M3����O�������Ɏ����I�Ɏg�p����X�^�b�N�T�C�Y
#define	UPUSHSIZE	(TSTKSIZE-ESTKSIZE)

//
//�@�^�X�N�̏�ԃR�[�h
//
#define	STATE_FREE	0x00
#define	STATE_IDLE	0x01
#define	STATE_READY	0x02
#define	STATE_SLEEP	0x03

#define	EOQ	0xff	// End Of Queue�F�L���[�̍Ō�ł��邱�Ƃ������B

#define	MAX_TASKNUM	8


//
// TCB�iTask Control Block)�̒�`
//
struct TCTRL_STRUC {
	unsigned char	link;
	unsigned char	state;
	unsigned char	msg_q;
	unsigned char	param0;
	TSTK		*sp;
	unsigned int	param1;
	unsigned int	param2;
};
typedef struct TCTRL_STRUC TCTRL;	// typedef����ƌ����ڂ�������Ɗi�D�ǂ��������Ă������x
TCTRL	tcb[MAX_TASKNUM];	// TCB���^�X�N�����m��

#define	STKSIZE	64			// �^�X�N�p�X�^�b�N�T�C�Y�i64���[�h�F256�o�C�g�j
unsigned int	stk_task[MAX_TASKNUM][STKSIZE];	// �^�X�N�p�X�^�b�N�G���A


//
// �Z�}�t�H
//
#define	MAX_SEMA	8
unsigned char semadat[MAX_SEMA];

//
// ���b�Z�[�W�u���b�N
//
// ���b�Z�[�W���L���[�ŊǗ�����ق����������i�D���ǂ��̂����ǂ��A
// ����̓��b�Z�[�W�u���b�N�̑��������Ȃ��̂ŁA�����̒P������D��
// ���āA����S���b�Z�[�W�u���b�N���X�L�������邱�Ƃɂ���
//
#define	MAX_MSGBLK	8
struct MSGBLK_STRUC {
	unsigned char	link;
	unsigned char	param_c;
	unsigned short	param_s;
	unsigned int	param_i;
};
typedef struct MSGBLK_STRUC MSGBLK;
MSGBLK msgblk[MAX_MSGBLK];



static LPC_GPIO_TypeDef (* const LPC_GPIO[4]) = { LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3 };
#define	LED_PORT	0
#define	LED_BIT		7



//===============================================
//= 		TCB�֌W����			=
//===============================================
unsigned char	q_pending[2];	// �����҂��^�X�N�iON/OFF�����ȂǂŎg�p�j
unsigned char	q_ready;	// �N�����
unsigned char	q_sleep;	// �X���[�v��ԁi�^�C�}�҂��j

unsigned char task_start;
unsigned char c_tasknum;	// ���݃X�P�W���[�����̃^�X�N�ԍ�
TCTRL	*c_task;


//
// �L���[�̍Ō�Ɏw�肳�ꂽTCB���Ȃ�
//
unsigned char tcbq_append(unsigned char *queue, unsigned char tcbnum)
{
	unsigned char ctasknum, ptasknum;
	if (tcbnum == EOQ)			// EOQ��������Ȃɂ����Ȃ�
		return(EOQ);
	if ((ctasknum = *queue) == EOQ) {	// ���ܐڑ�����Ă����͖���
		*queue = tcbnum;		// �^����ꂽTCB���Ȃ�
	} else {
		do {				// �Ō��T��
			ptasknum = ctasknum;	// PreviousTCB��CurrentTCB�ԍ���ۑ�
		} while((ctasknum = tcb[ptasknum].link) != EOQ);	// ptasknum���Ō�H
		tcb[ptasknum].link = tcbnum;	// �Ō��TCB�Ɏw�肳�ꂽTCB���Ȃ�
	}
	tcb[tcbnum].link = EOQ;			// �L���[�̏I���ɂȂ�̂ŁAEOQ
	return(tcbnum);
}

//
// �L���[�̐擪�����o��
//
unsigned char tcbq_get(unsigned char *queue)
{
	unsigned char tcbnum;
	if ((tcbnum = *queue) != EOQ)		// �L���[�̐擪�����o��
		*queue = tcb[tcbnum].link;	// EOQ�łȂ��Ȃ�A�����N��ɂȂ��Ȃ���
	return(tcbnum);				// �����A�L���[�̐擪��EOQ�Ȃ�EOQ���Ԃ邾��
}

//
// �w�肳�ꂽTCB���L���[����O��
//
unsigned char tcbq_remove(unsigned char *queue, unsigned char tcbnum)
{
	unsigned char ctasknum,ptasknum;
	ctasknum = *queue;
	if (tcbnum == EOQ)			// EOQ��������Ȃɂ����Ȃ�
		return(EOQ);
	if (ctasknum == EOQ)	// �L���[�ɉ����Ȃ����ĂȂ�
		return(EOQ);	// �Ȃ�EOQ�ŏI��
	if (ctasknum == tcbnum) {	//�@�����Ȃ�擪
		*queue = tcb[tcbnum].link;	// �Ȃ��ς���
		return(ctasknum);		// �I��
	}
	do {						// �}�b�`������̂�T�����[�v
		ptasknum = ctasknum;			// Previous��Current�̒l���R�s�[
		if ((ctasknum = tcb[ptasknum].link) != EOQ)	// �����N�悪EOQ��������
			return(EOQ);			// EOQ��Ԃ�
	} while(ctasknum != tcbnum);			// �����N�悪��v����܂Ń��[�v
	tcb[ptasknum].link = tcb[ctasknum].link;	// ctasknum�̃����N���q���ւ�
	return(ctasknum);
}

//
// �^�X�NON����
//
unsigned char process_taskon(unsigned char tasknum)
{
	unsigned char c;
	c = tcb[tasknum].state;
	if ((c == STATE_FREE) || (c == STATE_IDLE)) {		// Free��Idle�̂��̐�p
		tcb[tasknum].state = STATE_READY;
		tcbq_append(&q_ready,tasknum);
		return(tasknum);
	}
	return(EOQ);
}

//
// �^�X�NOFF����
//�@�@�w�肳�ꂽ�^�X�N��READY�܂���SLEEP��Ԃł���΁A�͂�����IDLE�X�e�[�g�ɂ���
//
unsigned char process_taskoff(unsigned char tasknum)
{
	unsigned char c;
	switch(c = tcb[tasknum].state) {
		case	STATE_READY:
			tcbq_remove(&q_ready, tasknum);
			tcb[tasknum].state = STATE_IDLE;
			return(tasknum);
		case	STATE_SLEEP:
			tcbq_remove(&q_sleep, tasknum);
			tcb[tasknum].state = STATE_IDLE;
			return(tasknum);
		default:
			return(EOQ);
	}
}

//
// �X���[�v����
//
void process_sleep(void)
{
	unsigned char tasknum;
	tasknum = q_sleep;
	while(tasknum != EOQ) {		// �L���[�̍Ō�܂ŃX�L����
		if (tcb[tasknum].param1 == 0) {	//�@�^�C���A�b�v����
			tcbq_remove(&q_sleep, tasknum);	// Sleep_Q����O��
			tcb[tasknum].state = STATE_READY;	// READY��Ԃɂ���
			tcbq_append(&q_ready, tasknum);	// Ready_Q�ɂȂ�
		} else {
			tcb[tasknum].param1--;	// �f�N�������g
		}
		tasknum = tcb[tasknum].link;
	}
}


//===============================================
//= ���b�Z�[�W�u���b�N�֘A����			=
//===============================================
unsigned char q_msgblk;

//
// �V�X�e���^�C�}���荞�݂̏���
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
// PendSV�̃n���h���i�^�X�N�X�C�b�`���O�̎��s�j
//
// �Ƃ���ŁA
// __attribute__ ((naked));
// �́A
// indicate that the specified function does not need prologue/epilogue
// sequences generated by the compiler
// �Ƃ������ƂŁA�R���p�C���ɂ�郌�W�X�^�ޔ��Ȃǂ��s�킹�Ȃ����߂̃A�g���r���[�g�B
// �R���p�C���Ŏg���X�^�b�N��̃��[�N�̈�Ȃǂ������m�ۂ���R�[�h���������Ȃ��̂�
// asm()�̒��ŏ����Ă��Ȃ���΂Ȃ�Ȃ��B
// �R���p�C�����ǂꂾ���X�^�b�N�̈���g���̂��͎���__attribute__���O���ăR���p�C��
// ���Ă݂Ċm�F���邵���Ȃ��B
//
unsigned char swstart = 0;
void schedule(void)
{
	unsigned char ctasknum,ntasknum;
	// �f�o�b�O�p�ɃX�C�b�`���O�񐔂��J�E���g
	pendsv_count++;
	// c_task��؂�ւ��i�^�X�N�X�C�b�`���O�j
	if (swstart) {
		if (rq_timer) {
			process_sleep();
			rq_timer = 0;
		}
		if ((ctasknum = q_pending[0]) !=EOQ) {		// Pending����Ă���^�X�N���䏈��������
			switch(q_pending[1]) {
				case STATE_READY:		// Ready��Ԃɂ�����
					process_taskon(ctasknum);	// TASKON����������
					break;
				case STATE_IDLE:		// Idle��Ԃɂ�����
					process_taskoff(ctasknum);
					break;
				default:
					break;
			}
			q_pending[0] = EOQ;			// ���������̂ŁA�N���A
		}
		ctasknum = c_tasknum;			// ���܂ŃX�P�W���[�����Ă����^�X�N�ԍ���ޔ�
		ntasknum = tcbq_get(&q_ready);		// Ready_Q������o��
		if (ntasknum == EOQ) {			// Ready_Q������
			if (c_tasknum == EOQ)		// �X�P�W���[�����Ă������̂��Ȃ��I�H�H
				c_tasknum = 0;		// ���肦�Ȃ����ǁA0�ɂ��Ă���
		} else {					// Ready_Q�ɂȂ����Ă���
			c_tasknum = ntasknum;			// Ready_Q������o�����̂��X�P�W���[����Ԃɂ���
			switch(tcb[ctasknum].state) {
				case	STATE_READY:	tcbq_append(&q_ready, ctasknum); break;
				case	STATE_SLEEP:	tcbq_append(&q_sleep, ctasknum); break;
				default:		break;
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
	// �O�������ł́A
	// �E�����ޔ�����Ȃ��ėp���W�X�^��R12��stmdb�iDec. Before)���g����PSP��ɑޔ�
	// �Ec_task�i���ݎ��s���̃^�X�N��TCB�������Ă���j��R12��ޔ�
	// �����s
	asm(						// R12�����[�N�p�X�^�b�N�Ƃ��ė��p
		"mrs	r12,psp\n\t"			// R12��PSP�̒l���R�s�[
//		"stmdb	r12!,{r4-r11,lr}\n\t"
		"stmdb	r12!,{r4-r11}\n\t"		// �����ޔ�����Ȃ�R4�`R11��ޔ�
		"movw	r2,#:lower16:c_task\n\t"	// *(ctask->sp) = R12;
		"movt	r2,#:upper16:c_task\n\t"
		"ldr	r0,[r2,#0]\n\t"
		"str	r12,[r0,#4]\n\t"
	);

	// ���ɃX�P�W���[������^�X�N�̑I��
	asm(
		"push	{lr}\n\t"
		"bl	schedule\n\t"
		"pop	{lr}\n\t"
	);

	// �㔼�����ł�
	// �V����c_task�̎w����i���ɓ������^�X�N��TCB�j���烌�W�X�^�𕜋A
	// ���x��
	// �ER12�����o��
	// �E�ėp���W�X�^�𕜋��ildmia�iInc. After�j�𗘗p�j
	// ���ɖ߂�
	asm (
		"movw	r2,#:lower16:c_task\n\t"	// R12 = *(c_task->sp);
		"movt	r2,#:upper16:c_task\n\t"
		"ldr	r0,[r2,#0]\n\t"
		"ldr	r12,[r0,#4]\n\t"

//		"ldmia	r12!,{r4-r11,lr}\n\t"
		"ldmia	r12!,{r4-r11}\n\t"		// R4�`R11�𕜋A

		"msr	psp,r12\n\t"			// PSP = R12;
		"bx	lr\n\t"				// (RETURN)
	);
}

unsigned int svcop;
unsigned int svcparam[2];
void SVCall_Handler(void) __attribute__ ((naked));
void SVCall_Handler()
{
	asm(
		"movw	r2,#:lower16:svcparam\n\t"	// svcparam[0] = R0;
		"movt	r2,#:upper16:svcparam\n\t"	// svcparam[1] = R1;
		"str	r0,[r2,#0]\n\t"
		"str	r1,[r2,#4]\n\t"
		"mov	r0,lr\n\t"		// if ((R0 = LR & 0x04) != 0) {
		"ands	r0,#4\n\t" 		//			// LR�̃r�b�g4��'0'�Ȃ�n���h�����[�h��SVC
		"beq	.L0000\n\t"		//			// '1'�Ȃ�X���b�h���[�h��SVC
		"mrs	r1,psp\n\t"		// 	R1 = PSP;	// �v���Z�X�X�^�b�N���R�s�[
		"b	.L0001\n\t"		//
		".L0000:\n\t"			// } else {
		"mrs	r1,msp\n\t"		//	R1 = MSP;	// ���C���X�^�b�N���R�s�[
		".L0001:\n\t"			// }
		"ldr	r2,[r1,#24]\n\t"	// R2 = R1->PC;
		"ldr	r0,[r2,#-2]\n\t"	// R0 = *(R2-2);	// SVC(SWI)���߂̉��ʃo�C�g����������

		"movw	r2,#:lower16:svcop\n\t"	// svcop = R0;		// svcop�ϐ��ɃR�s�[
		"movt	r2,#:upper16:svcop\n\t"
		"str	r0,[r2,#0]\n\t"

		"push	{r7}\n\t"		// PUSH R7	// C����Ńt���[���|�C���^��R7���g���Ă��邽��
		"sub	sp,sp,#8\n\t"		// SP -= 8;	// (4�o�C�g�~2���j�FC���ꑤ�łQ���[�h���g���Ă�������
		"mov	r7,sp\n\t"		// R7 = SP;
	);

	switch(svcop & 0xff) {			// SVC�̈����i���N�G�X�g�R�[�h�j���擾
		case 0x00:	// NULL�i�ăX�P�W���[�����O���邾���j
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;		// PendSV�𔭐������ăX�P�W���[�����O
			break;
		case 0x01:	// LEDOFF
			LPC_GPIO[LED_PORT]->MASKED_ACCESS[1<<LED_BIT] = 0;
			break;
		case 0x02:	// LEDON
			LPC_GPIO[LED_PORT]->MASKED_ACCESS[1<<LED_BIT] = -1;
			break;
		case 0x10:	// SLEEP
			tcb[c_tasknum].param1 = svcparam[0];		// �p�����[�^��ς��
			tcb[c_tasknum].state = STATE_SLEEP;		// �X���[�v��Ԃɂ��Ă���
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;		// PendSV�𔭐������ăX�P�W���[�����O
			break;
		case 0x11:	// TASKON
			q_pending[0] = svcparam[0];			// �^�X�N�ԍ���Pending�ɃZ�b�g
			q_pending[1] = STATE_READY;			// READY��ԂɑJ�ڂ�������
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;		// PendSV�𔭐������ăX�P�W���[�����O
			break;
		case 0x12:	// TASKOFF
			q_pending[0] = svcparam[0];			// �^�X�N�ԍ���Pending�ɃZ�b�g
			q_pending[1] = STATE_IDLE;			// IDLE��ԂɑJ�ڂ�������
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;		// PendSV�𔭐������ăX�P�W���[�����O
			break;
		case 0x13:	// SEMAGET
			asm (
				"movw	r2,#:lower16:svcparam\n\t"
				"movt	r2,#:upper16:svcparam\n\t"
				"ldr	r0,[r2,#0]\n\t"
				"movw	r2,#:lower16:semadat\n\t"
				"movt	r2,#:upper16:semadat\n\t"
				"add	r2,r0\n\t"
				"mov	r0,#0\n\t"
				"ldrb	r0,[r2,#0]\n\t"
				"str	r0,[r1,#0]\n\t"
				"cmp	r0,#0\n\t"
				"bne	.L0100\n\t"
				"mov	r0,#1\n\t"
				"str	r0,[r2,#0]\n\t"
				".L0100:\n\t"
			);
			break;
		case 0x14:	// CLRSEMA
			semadat[svcparam[0]] = 0;
			break;
		case 0x15:	// MSGBLKGET
			asm (
				"movw	r3,#:lower16:q_msgblk\n\t"	// R3 = &q_msgblk;
				"movt	r3,#:upper16:q_msgblk\n\t"
				"ldrb	r0,[r3,#0]\n\t"			// R0 = q_msgblk;
				"cmp	r0,#0xff\n\t"			// if (R0 != EOQ) {
				"beq	.L1500\n\t"			//
				"movw	r2,#:lower16:msgblk\n\t"	//   R2 = msgblk[R0].link;
				"movt	r2,#:upper16:msgblk\n\t"
				"ldrb	r2,[r2, r0, lsl #3]\n\t"
				"strb	r2,[r3, #0]\n\t"		//   q_msgblk = R2;
				"movw	r2,#:lower16:msgblk\n\t"	//   msgblk[R0].link = 0xff;
				"movt	r2,#:upper16:msgblk\n\t"
				"mov	r3,#0xff\n\t"
				"strb	r3,[r2, r0, lsl #3]\n\t"
				".L1500:\n\t"				// }
				"str	r0,[r1,#0]\n\t"			// return(R0);
			);
			break;
		case 0x16:	// MSGBLKFREE
			asm (
				"movw	r2,#:lower16:svcparam\n\t"
				"movt	r2,#:upper16:svcparam\n\t"
				"ldrb	r1,[r2,#0]\n\t"
				"movw	r2,#:lower16:msgblk\n\t"
				"movt	r2,#:upper16:msgblk\n\t"

				"movw	r3,#:lower16:q_msgblk\n\t"
				"movt	r3,#:upper16:q_msgblk\n\t"
				"ldrb	r0,[r3,#0]\n\t"
				"strb	r1,[r3,#0]\n\t"
				"strb	r0,[r2, r1, lsl #3]\n\t"
			);
			break;
		case 0x17:	// MSGBLKSEND(task#, msgblk#);
			asm (
				"movw	r3,#:lower16:svcparam\n\t"
				"movt	r3,#:upper16:svcparam\n\t"
				"ldr	r0,[r3,#0]\n\t"		// R0=svcparam[0]<<4;(TASK#)
				"mov	r0,r0, lsl #4\n\t"
				"ldr	r1,[r3,#4]\n\t"		// R1=svcparam[1];(MSGBLK#)
				"movw	r3,#:lower16:tcb\n\t"
				"movt	r3,#:upper16:tcb\n\t"
				"add	r0,#2\n\t"
				"ldrb	r2,[r3,r0]\n\t"		// R2 = tcb[TASK#].msg_q;(MSGBLK#)
				"cmp	r2,#0xff\n\t"		// if (R2 == EOQ) {
				"bne	.L1700\n\t"
				"strb	r1,[r3,r0]\n\t"		// tcb[svcparam[0]].msg_q = R1;
				"b	.L1702\n\t"		// } else {
				".L1700:\n\t"
				"movw	r3,#:lower16:msgblk\n\t"
				"movt	r3,#:upper16:msgblk\n\t"
				".L1701:\n\t"			// while(1) {
				"ldrb	r0,[r3,r2, lsl #3]\n\t"	//   R0 = msgblk[R2].link
				"cmp	r0,#0xff\n\t"		//   if (R0 == EOQ)
				"beq	.L1702\n\t"		//     break;
				"mov	r2,r0\n\t"		//   R2 = R0;
				"b	.L1701\n\t"		// }
				".L1702:\n\t"			//}
				"strb	r1,[r3,r2, lsl #3]\n\t"	// msgblk[R2].link = R1;
				"movw	r3,#:lower16:msgblk\n\t"	// msgblk[R1].link = EOQ;
				"movt	r3,#:upper16:msgblk\n\t"
				"mov	r0,#0xff\n\t"
				"strb	r0,[r3,r1, lsl #3]\n\t"
			);
			break;
		case 0x18:	// MSGBLKRCV
			asm (
				"movw	r2,#:lower16:c_tasknum\n\t"	// R3 = &(tcb[c_tasknum].msg_q);
				"movt	r2,#:upper16:c_tasknum\n\t"
				"ldrb	r2,[r2,#0]\n\t"
				"mov	r2,r2, lsl #4\n\t"
				"add	r2,#2\n\t"
				"movw	r3,#:lower16:tcb\n\t"
				"movt	r3,#:upper16:tcb\n\t"
				"add	r3,r2\n\t"

				"ldrb	r0,[r3,#0]\n\t"			// R0 = tcb[c_tasknum].msg_q;
				"cmp	r0,#0xff\n\t"			// if (R0 != EOQ) {
				"beq	.L1800\n\t"
				"movw	r2,#:lower16:msgblk\n\t"	//   tcb[c_tasknum].msg_q = msgblk[R0].link;
				"movt	r2,#:upper16:msgblk\n\t"
				"ldrb	r2,[r2,r0, lsl #3]\n\t"
				"strb	r2,[r3,#0]\n\t"
				".L1800:\n\t"				// }
				"str	r0,[r1,#0]\n\t"			// return(R0);
			);
			break;
		case	0x19:	// TASKIDGET
			asm(
				"movw	r0,#:lower16:c_tasknum\n\t"	// R0=c_tasknum
				"movt	r0,#:upper16:c_tasknum\n\t"
				"ldrb	r0,[r0,#0]\n\t"
				"str	r0,[r1,#0]\n\t"			// return(R0);
			);
			break;
		case 0xf0:	// CHG_IDLE
			tcb[c_tasknum].state = STATE_IDLE;
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;		// PendSV�𔭐������ăX�P�W���[�����O
			break;
		case 0xff:	// CHG_UNPRIVILEGE
			asm(
				"movw	r2,#:lower16:c_task\n\t"	// psp = *(c_task->sp);
				"movt	r2,#:upper16:c_task\n\t"
				"ldr	r0,[r2,#0]\n\t"
				"ldr	r2,[r0,#4]\n\t"
				"msr	psp,r2\n\t"
				"orr	lr,lr,#4"			// LR |= 0x04;
									// �X���b�h���[�h�Ɉڍs
									// 1001:msp�g�p(�v���Z�X�j 1101:psp�g�p�i�X���b�h�j
									// �Ȃ̂ŁA�Z�b�g����ƃX���b�h���[�h�ɂȂ�
			);
			task_start = 1;					// �ȍ~�̓^�X�N�X�C�b�`���O���s
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;		// PendSV�𔭐�������
			break;
		default:
			break;
	}
	asm(	"add	sp,sp,#8\n\t"
		"pop	{r7}\n\t"
		"bx	lr\n\t"
	);
}


//=======================================
//= ��������̓^�X�N			=
//=======================================
//

//===============================
//=				=
//= SVC�̒�`			=
//=				=
//===============================


#define	SYSCALL_NULL	asm("svc #0x00\n\t")
#define	SYSCALL_LEDOFF	asm("svc #0x01\n\t")
#define	SYSCALL_LEDON	asm("svc #0x02\n\t")

#define	SYSCALL_IDLE	asm("svc #0xf0\n\t")
#define	SYSCALL_CHG_UNPRIVILEGE	asm("svc #0xff\n\t")

#define SYSCALL_SLEEP(x)	{register int p0 asm("r0"); p0=x; asm ("svc #0x10"::"r"(p0));}
#define	SYSCALL_TASKON(x)	{register int p0 asm("r0"); p0=x; asm ("svc #0x11"::"r"(p0));}
#define	SYSCALL_TASKOFF(x)	{register int p0 asm("r0"); p0=x; asm ("svc #0x12"::"r"(p0));}
#define	SYSCALL_SEMAGET(x)	{register int p0 asm("r0"); p0=x; asm ("svc #0x13"::"r"(p0));}
#define	SYSCALL_SEMACLR(x)	{register int p0 asm("r0"); p0=x; asm ("svc #0x14"::"r"(p0));}
#define	SYSCALL_MSGBLKGET	asm("svc #0x15\n\t")
#define	SYSCALL_MSGBLKFREE(x)	{register int p0 asm("r0"); p0=x; asm ("svc #0x16"::"r"(p0));}
#define	SYSCALL_MSGBLKSEND(tcb,msgblk)	{register int p0 asm("r0"); register int p1 asm ("r1"); p0=tcb; p1=msgblk;  asm ("svc #0x17"::"r"(p0,p1));}
#define	SYSCALL_MSGBLKRCV	asm("svc #0x18\n\t")
#define	SYSCALL_TASKIDGET	asm("svc #0x19\n\t")


//===============================
//=				=
//= SVC�C���^�[�t�F�[�XAPI	=
//=				=
//===============================

//
// NULL:�X�P�W���[�����Ăяo���A���̃^�X�N���N������
//
void SVC_NULL(void)
{
	SYSCALL_NULL;
}

//
// LEDOFF�F�{�[�h���LED����������
//
void SVC_LEDOFF(void)
{
	SYSCALL_LEDOFF;
}


//
// LEDON�F�{�[�h���LED��_������
//
void SVC_LEDON(void)
{
	SYSCALL_LEDON;
}

//
// IDLE:IDLE�X�e�[�g�Ɉڍs����i���b�Z�[�W�҂��Ȃǂŗ��p�j
//
void SVC_IDLE(void)
{
	SYSCALL_IDLE;
}

//
// �X���b�h���[�h�Ɉڍs�imain()����MicroMulti���N�����鎞�ɗ��p���邾���j
//
void SVC_CHG_UNPRIVILEGE(void)
{
	SYSCALL_CHG_UNPRIVILEGE;
}

//
// SLEEP�F�w�肵���񐔕��V�X�e���^�C�}���荞�݂�����܂ŃX���[�v
//
void SVC_SLEEP(unsigned int times)
{
	SYSCALL_SLEEP(times);
}

//
// TASKON�F�w�肵���^�X�N�ԍ��̃^�X�N���N��
//
void SVC_TASKON(unsigned int tasknum)
{
	SYSCALL_TASKON(tasknum);
}

//
// TASKOFF�F�w�肵���^�X�N�ԍ��̃^�X�N���~
//
void SVC_TASKOFF(unsigned int tasknum)
{
	SYSCALL_TASKOFF(tasknum);
}

//
// SEMAGET�F�w�肵���ԍ��̃Z�}�t�H�擾�v��
//�@�Z�}�t�H�������0�A���Ȃ����0�ȊO���Ԃ�
//
unsigned char SVC_SEMAGET(unsigned char d)
{
	register unsigned int retval asm ("r0");	// retval��R0���W�X�^�Ɋ��t��
	SYSCALL_SEMAGET(d);
	return(retval);
}

//
// �Z�}�t�H������܂ő҂�
//
void SVC_SEMAGET_W(unsigned char d)
{
	while(SVC_SEMAGET(d))
		;
}


//
// SEMACLR�F�w�肵���ԍ��̃Z�}�t�H���N���A�i�ԋp�j
//
void SVC_SEMACLR(unsigned char d)
{
	SYSCALL_SEMACLR(d);
}

//
// MSGBLKGET:�t���[�ȃ��b�Z�[�W�u���b�N�����o��
//
unsigned char SVC_MSGBLKGET()
{
	register unsigned char c asm("r0");
	SYSCALL_MSGBLKGET;
	return(c);
}

// ���b�Z�[�W�u���b�N���擾�ł���܂ŃE�F�C�g����^�C�v
unsigned char SVC_MSGBLKGET_W()
{
	unsigned char c;
	while((c = SVC_MSGBLKGET()) == EOQ)
		;
	return(c);
}

//
// MSGBLKFREE�F�w�肵�����b�Z�[�W�u���b�N��ԋp
//
void SVC_MSGBLKFREE(unsigned char d)
{
	SYSCALL_MSGBLKFREE(d);
}

//
// MSGBLKSND�F�w�肵���^�X�N�Ƀ��b�Z�[�W�u���b�N�𑗐M
//
void SVC_MSGBLKSEND(unsigned char tcbnum, unsigned char msgblk)
{
	SYSCALL_MSGBLKSEND(tcbnum,msgblk);
}

//
// MSGBLKRCV�F���b�Z�[�W��M
//�@���b�Z�[�W������΃��b�Z�[�W�ԍ��A�������EOQ�i0xff�j���Ԃ�
//
unsigned char SVC_MSGBLKRCV()
{
	register unsigned char c asm("r0");
	SYSCALL_MSGBLKRCV;
	return(c);
}


//
// 
//
unsigned char SVC_TASKIDGET()
{
	register unsigned char c asm("r0");
	SYSCALL_TASKIDGET;
	return(c);
}

//===============================
//=				=
//=�@�^�X�N�֐�			=
//=				=
//===============================

unsigned char dbgdata[3];
void th_zero()
{
	dbgdata[0] = SVC_TASKIDGET();
	while(1) {
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
		mblk = SVC_MSGBLKGET_W();
		msgblk[mblk].param_c = 1;
		SVC_MSGBLKSEND(2, mblk);
		SVC_SLEEP(2);
		mblk = SVC_MSGBLKGET_W();
		msgblk[mblk].param_c = 0;
		SVC_MSGBLKSEND(2, mblk);
		SVC_SLEEP(2);
		mblk = SVC_MSGBLKGET_W();
		msgblk[mblk].param_c = 2;
		SVC_MSGBLKSEND(2, mblk);
		SVC_SLEEP(2);
	}
}

void th_ledon()
{
	unsigned int i,j,k;
	unsigned char mblk;
	dbgdata[2] = SVC_TASKIDGET();
	while(1) {
		SVC_IDLE();
		do {
			mblk = SVC_MSGBLKRCV();
		} while(mblk == EOQ);
		switch(msgblk[mblk].param_c) {
			case	0x00:	SVC_LEDOFF();
					break;
			case	0x01:	SVC_LEDON();
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
//=				=
//=�@�X�^�[�g�A�b�v�Ə������֐�	=
//=				=
//===============================

//
// ���b�Z�[�W�u���b�N�̏�����
//
//  �S�u���b�N�Ƃ��t���[��Ԃɂ��邾��
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
// �Z�}�t�H�f�[�^�̏�����
//
void init_semadat()
{
	unsigned char c;
	for (c=0; c<MAX_SEMA; c++)
		semadat[c] = 0;
}

//
//  TCB�̏������ƃ^�X�N�o�^
//
//�@XPSR�̃r�b�g24��'1'�ɂ��Ȃ��ƃn�[�h�G���[�ɂȂ�̂Œ���
//
//�@�X�^�b�N�f�[�^�쐬�����ڃL���X�g���ăA�N�Z�X�����
//�@�R�[�h�������ԈႤ�悤�Ȃ̂ŁA��Uunsigned int��
//�@�L���X�g���đ�����Ȃ������B
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
	p+=(STKSIZE-TSTKSIZE);
	p_stk = (TSTK*)(p);
	p_stk->r_r0 = 0x00;
	p_stk->r_r1 = 0x01;
	p_stk->r_r2 = 0x02;
	p_stk->r_lr = 0x00;
	p_stk->r_pc = (unsigned int)(task);
	p_stk->r_xpsr = 0x01000000;
	tcb[tasknum].sp = (TSTK*)(p);
}

//
// �������烁�C�����[�`��
// ���X�̏����������Ă���A�}���`�^�X�N���[�h�Ɉڍs
//
int main(void)
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
	LPC_GPIO[LED_PORT]->DIR |= 1<<LED_BIT;
	LPC_GPIO[LED_PORT]->MASKED_ACCESS[1<<LED_BIT] = 0;

	pendsv_count = 0;
	systick_count = 10;
//	SysTick_Config(SystemCoreClock/100);	// 1/100�b�i=10ms�j���Ƃ�SysTick���荞��
	SysTick_Config(SystemCoreClock/10);	// 1/10�b�i=100ms�j���Ƃ�SysTick���荞��
	NVIC_SetPriority(SVCall_IRQn, 0x80);	// SVC�̗D��x�͒��ق�
	NVIC_SetPriority(SysTick_IRQn, 0xc0);	// SysTick�̗D��x��SVC���Ⴍ
	NVIC_SetPriority(PendSV_IRQn, 0xff);	// PendSV�̗D��x���Œ�ɂ��Ă���
	NVIC_EnableIRQ(SysTick_IRQn);

	// Enter an infinite loop, just incrementing a counter
	for (i=0; i<10; i++) {
		while(systick_count)
			;
//		systick_count = 20;
		systick_count = 2;
		SVC_LEDON();
		while(systick_count)
			;
//		systick_count = 20;
		systick_count = 2;
		SVC_LEDOFF();
	}

	//�@���̒i�K�ł͂܂��������[�h�ł���AMSP���g���Ă���
	//�@PSP�v���Z�X�X�^�b�N�͔�������[�h�œ����Ă�����
	// �^�X�N#0�imain()���璼�ڐ؂�ւ���ē����^�X�N�j�̃X�^�b�N
	// �ɂ��ẮA���[�U���ςޕ���������Ă���
	// CHG_UNPRIVILEGE��SVC=>PendSV�̎���PSP�ɐς݂Ȃ������Ƃ�
	// ���܂�����
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

	// Ready_Q�ɂȂ��ł���
	// �ŏ���#0���������Ƃɂ����̂ŁA#0�͂����Ȃ�CurrentTASK�ɂȂ邽�߁A
	// Ready_Q�ɂȂ��̂�0�ȊO
	tcbq_append(&q_ready, 1);
//	tcbq_append(&q_ready, 2);

	tcb[3].state = STATE_FREE;
	tcb[3].link = EOQ;

	SVC_CHG_UNPRIVILEGE();	// �X���b�h#0�Ɉڍs����

	while(1);		// �߂��Ă��邱�Ƃ͖������ǁA�O�̂���
}

