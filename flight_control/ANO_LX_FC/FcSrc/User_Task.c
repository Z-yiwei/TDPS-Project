#include "User_Task.h"
#include "Drv_RcIn.h"
#include "LX_FC_Fun.h"

#define FAKE_RC_ENABLE   1          /* 0 = disable fake-RC logic          */
#define CH_NUM           4          /* total radio channels               */

#define PULSE_MID        1500       /* 1300-1700 ? �take-off� trigger     */
#define PULSE_START      2000       /* land pulse (optional)              */
#define PULSE_LOW        1000 

#define T_WAIT_MS        30000       /* power-on settle time before unlock */
#define T_HOVER_MS       1000       /* hover duration before landing      */


void UserTask_OneKeyCmd(void)
{
    //////////////////////////////////////////////////////////////////////
    //????/????
    //////////////////////////////////////////////////////////////////////
    //???????????/?????????
    static u8 one_key_takeoff_f = 1, one_key_land_f = 1, one_key_mission_f = 0;
    static u8 mission_step;
    //??????????
    if (rc_in.no_signal == 0)
    {
        //???6?????? 1300<CH_6<1700
        if (rc_in.rc_ch.st_data.ch_[ch_6_aux2] > 1300 && rc_in.rc_ch.st_data.ch_[ch_6_aux2] < 1700)
        {
            //预留
        }
        else
        {
            //????,??????
            one_key_takeoff_f = 0;
        }
        //
        //???6?????? 800<CH_6<1200
        if (rc_in.rc_ch.st_data.ch_[ch_6_aux2] > 800 && rc_in.rc_ch.st_data.ch_[ch_6_aux2] < 1200)
        {
            //?????
            if (one_key_land_f == 0)
            {
                //??????
                one_key_land_f =
                    //??????
                    OneKey_Land();
            }
        }
        else
        {
            //????,??????
            one_key_land_f = 0;
        }
	//???6?????? 1700<CH_6<2200 
		if(rc_in.rc_ch.st_data.ch_[ch_6_aux2]>1700 && rc_in.rc_ch.st_data.ch_[ch_6_aux2]<2200)
		{
			//?????
			if(one_key_mission_f ==0)
			{
				//??????
				one_key_mission_f = 1;
				//????
				mission_step = 1;
			}
		}
		else
		{
			//????,??????
			one_key_mission_f = 0;		
		}
		//
		if(one_key_mission_f==1)
		{
			static u16 time_dly_cnt_ms;
			//
			switch(mission_step)
			{
				case 0:
				{
					//reset
					time_dly_cnt_ms = 0;
				}
				break;
				case 1:
				{
					//??????

					mission_step = 2;
				}
				break;
				case 2:
				{
						if (FC_Unlock())
						                  /* 解锁成功 → 继续流程          */
						{
							mission_step = 3;             /* 进入下一阶段（延时 2 s）      */
						}
						else                              /* 解锁仍未完成 → 保持本状态    */
						{
							mission_step = 2;             /* 留在 case 1，下一帧再尝试    */
						}
				}
				break;
				case 3:
				{
					if(time_dly_cnt_ms<2000)
					{
						time_dly_cnt_ms+=20;//ms
					}
					else
					{
						time_dly_cnt_ms = 0;
						mission_step += 1;
					}
				}
				break;
				case 4:
				{
					//??
					mission_step += OneKey_Takeoff(100);//????:??; 0:???????????
				}
				break;
				case 5:
				{
					//?10?
					if(time_dly_cnt_ms<2000)
					{
						time_dly_cnt_ms+=20;//ms
					}
					else
					{
						time_dly_cnt_ms = 0;
						mission_step += 1;
					}					
				}
				break;
				case 6:
				{

					if(time_dly_cnt_ms< 15000)
					{
					  time_dly_cnt_ms+=20;//ms
						rt_tar.st_data.vel_x = 20;
					}	
					
					else
					{
						time_dly_cnt_ms = 0;
						mission_step += 1;
						rt_tar.st_data.vel_x = 0;
					}					
					
				}
				break;
				case 7:
				{
					if(time_dly_cnt_ms<3000)
					{
						time_dly_cnt_ms+=20;//ms
					}
					else
					{
						time_dly_cnt_ms = 0;
						mission_step += 1;
					}	
				}
				break;
				case 8:
				{

					if(time_dly_cnt_ms< 10000)
					{
					  time_dly_cnt_ms+=20;//ms
						rt_tar.st_data.vel_y = -20;
					}	
					
					else
					{
						time_dly_cnt_ms = 0;
						mission_step += 1;
						rt_tar.st_data.vel_y = 0;
					}					
					
				}
				break;
				
				case 9:
				{
					//??????
					OneKey_Land();					
				}
				break;	
							
				default:break;
			}
		}
		else
		{
			mission_step = 0;
		}
	}
    ////////////////////////////////////////////////////////////////////////
}

void FakeRc_Generator(void)
{
#if FAKE_RC_ENABLE
    /* do nothing when a real receiver is alive */
    if (rc_in.fail_safe == 0 && rc_in.no_signal == 0)
        return;

    uint32_t now = GetSysRunTimeMs();

    /* -------- 1. fabricate an RC frame -------- */
    rc_in.fail_safe = 0;
    rc_in.no_signal = 0;

    for (int i = 0; i < CH_NUM; ++i)
        rc_in.rc_ch.st_data.ch_[i] = 1500;      /* neutral */

	rc_in.rc_ch.st_data.ch_[4] = 1500;      /* neutral */

    /* -------- 2. CH6 pulse state-machine -------- */
    static enum {S_IDLE, S_START, S_DONE} s = S_IDLE;
    static uint32_t t_ref = 0;                  /* phase timer */

    switch (s) {

    case S_IDLE:                                /* wait for unlock */
        rc_in.rc_ch.st_data.ch_[ch_6_aux2] = PULSE_MID;
        if ( now > (T_WAIT_MS)) {
            s = S_START;
            t_ref = now;
        }
        break;

    case S_START:                             /* 0.3 s take-off pulse */
        rc_in.rc_ch.st_data.ch_[ch_6_aux2] = PULSE_START;
        if (now - t_ref > 300000) {
            t_ref = now;
			s = S_DONE;
        }
        break;

	case S_DONE:                             /* 0.3 s take-off pulse */
        if (now - t_ref > 30000000) {
            t_ref = now;
        }
        break;


    default:                                    /* finished */
        rc_in.rc_ch.st_data.ch_[ch_6_aux2] = PULSE_LOW;
        break;
    }
#endif
}
