#include "TTy.h"
#include "Assembly.h"
#include "Kernel.h"
#include "CRT.h"
/*==============================class TTy_Queue===============================*/
TTy_Queue::TTy_Queue()
{
	this->m_Head = 0;
	this->m_Tail = 0;
}

TTy_Queue::~TTy_Queue()
{
	//nothing to do here
}

char TTy_Queue::GetChar()
{
	char ch = TTy::GET_ERROR;
	{
		if ( this->m_Head == this->m_Tail )
		{
			//Buffer Empty
			return ch;
		}
	}

	ch = this->m_CharBuf[m_Tail];
	this->m_Tail = ( this->m_Tail + 1 ) % TTY_BUF_SIZE;

	return ch;
}

void TTy_Queue::PutChar(char ch)
{
	this->m_CharBuf[m_Head] = ch;
	this->m_Head = ( this->m_Head + 1 ) % TTY_BUF_SIZE;
}

int TTy_Queue::CharNum()
{
	/* ��Head < Tailʱʹ��%���������⣡������'&'���㡣
	 *  Ʃ���±�Head = 5��Tail = 10�� (5 - 10) %  TTY_BUF_SIZE ��
	 *  �ᱻ����0xFFFF FFFB  ( =4294967291) ȥģ����TTY_BUF_SIZE������ʹ��ˡ�
	 */
	// unsigned int ans = this->m_Head - this->m_Tail;
	// ans = ans % TTY_BUF_SIZE;
	// return ans;
	
	int ans = (this->m_Head - this->m_Tail) & (TTy_Queue::TTY_BUF_SIZE - 1);
	return ans;
}

char* TTy_Queue::CurrentChar()
{
	/* ������һ����Ҫȡ���ַ��ĵ�ַ */
	return &this->m_CharBuf[m_Tail];
}

/*==============================class TTy===============================*/
/* ����̨����ʵ���Ķ��� */
TTy g_TTy[2];

TTy::TTy()
{

}

TTy::~TTy()
{

}





/*
 * �ӱ�׼�������ȡ�ַ������û���
 * ֱ������Ϊ�գ�������Ӧ�ó���֮���裨u.u_IOParam.m_Count Ϊ  0��
 * ��䣬�����п�����˯����һ���� ��Ϊ��׼�������Ϊ�գ���ԭʼ���������û�ж����
 * ���û�û������س��������ܶ�ԭʼ�����е��������ݽ����޸ģ�
 * */
void TTy::TTRead()
{
	/* �豸û�п�ʼ���������� */
	if ( (this->t_state & TTy::CARR_ON) == 0 )
	{
		return;
	}

	if ( this->t_canq.CharNum() || this->Canon() )
	{
		while ( this->t_canq.CharNum() && (this->PassC(this->t_canq.GetChar()) >= 0) );
	}
}

/*
 * һ��һ���ֽڵؽ��û����еȴ�����������ͱ�׼������С�
 * ��������������������ˢ�����Դ档 ������̻��������ˢ�¡�
 * ����CRT::m_BeginChar����ָ����������д����һ���ַ��ĵ�Ԫ��BackSpace�����Բ�����ָ��֮ǰ���κ��ַ���
 */
void TTy::TTWrite()
{
	/* 
	 * ��Ϊ���ڵ�����豸���ڴ棬����Ӧ�ٶ��൱�죬����
	 * ����Ҫ�ڽ������Ժ����жϣ������Ĵ��۷ǳ������
	 * ԭ��unix v6����Щ��ͬ����������������ַ��Ĺ����й�
	 * �ж�����ֹ�û����뵫���ܱ�����ɾ����bug,����Ϊ����
	 * ���ܻᵼ��ʱ���ж���Ӧ���ӳ٣��������������أ���
	 * ��û����������
	 */
	char ch;
	
	 /* �豸û�п�ʼ���������� */
	if ( (this->t_state & TTy::CARR_ON) == 0 )
	{
		return;
	}

	while ( (ch = CPass()) > 0 )
	{
		/*��������г����涨�ַ�������Ҫ�Ͽ���ʾ */
		if ( this->t_outq.CharNum() > TTy::TTHIWAT)
		{
			this->TTStart();
			/* ��������BeginCharָ������ַ���������У�δȷ�ϲ��ֵ���ʼ����
			 * Ŀ�����ڲ�����Backspace��ɾ��д�ڱ�׼����ϵ����ݣ�Ʃ��������ʾ��֮�ࡣ
			 */
			if(this->ntty ==0)
			CRT::m_BeginChar = this->t_outq.CurrentChar();
			else
			CRT::m1_BeginChar = this->t_outq.CurrentChar();
		}
		this->TTyOutput(ch);
	}
	this->TTStart();
	if(this->ntty ==0)
		CRT::m_BeginChar = this->t_outq.CurrentChar();
	else
		CRT::m1_BeginChar = this->t_outq.CurrentChar();
	/* ����BeginCharΪ�˷�ֹ����ɾ����ӡ���ַ���������Ҫ�����ʾ���棬�������ǰ���������
	 * ���ַ��ڱ�ɾ��ʱ�����ܱ�ɾ����������ʵ�����Ѿ���ɾ���ˡ�
	 */
}

/* ���жϴ���������á�����ch��ɨ����ת���ɵ�ASCII�롣
 * ���ܣ���ch����ԭʼ������У���������л��� ���ͱ�׼������У�֮������TTStart���Դ棩
 * �����дֲڵĵط���this->t_rawq.PutChar(ch) ֮ǰû���ж�ԭʼ���������û��������ɵĽ����ԭʼ������ԭ������
 * ���ַ���ɾ����
 * ԭʼ���������ֻ��˵��һ�����⣺û�н����ڵȴ��������롣
 * ���� �Ľ�ϵͳ��TTyInput���������жϣ�û�н���˯�ߵȴ����������ʱ�򣬲�Ҫ��ch����ԭʼ���С�
 * */
void TTy::TTyInput(char ch)
{
	if ( (ch &= 0xFF) == '\r' && (this->t_flags & TTy::CRMOD) )
	{
		ch = '\n';
	}

	/* �����Сд�ն� */
	if ( (this->t_flags & TTy::LCASE) && ch >= 'A' && ch <= 'Z' )
	{
		ch += 'a' - 'A';
	}

	/* �������ַ�����ԭʼ�ַ�������� */
	this->t_rawq.PutChar(ch);

	if ( this->t_flags & TTy::RAW || ch == '\n' || ch == TTy::CEOT )
	{
		Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&this->t_rawq);
		this->t_rawq.PutChar(0x7);
		this->t_delct++;
	}

	if ( this->t_flags & TTy::ECHO )
	{
		this->TTyOutput(ch);
		this->TTStart();
	}

	else if(this->t_flags & TTy::RPW)
	{ //������Ļ�������*
		if(ch!='\n')
		{
			ch='*';
			this->TTyOutput(ch);
		}
		this->TTStart();
	}
}

void TTy::TTyOutput(char ch)
{
	/* ��������ַ�Ϊ�ļ��������������ն˹�����ԭʼ��ʽ�£��򷵻� */
	if ( (ch &= 0xFF) == TTy::CEOT && (this->t_flags & TTy::RAW) == 0 )
	{
		return;
	}

	if ( '\n' == ch && (this->t_flags & TTy::CRMOD) )
	{
		this->TTyOutput('\r');
	}

	/* ���ַ���������ַ���������� */
	if (ch)
	{
		this->t_outq.PutChar(ch);
	}
}
void TTy::TTStart()
{
	CRT::CRTStart(this,this->ntty);
}

void TTy::FlushTTy()
{
	while ( this->t_canq.GetChar() >= 0 );
	while ( this->t_outq.GetChar() >= 0 );
	Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&this->t_canq);
	Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&this->t_outq);
	
	X86Assembly::CLI();
	while ( this->t_rawq.GetChar() >= 0 );
	this->t_delct = 0;
	X86Assembly::STI();
}

int TTy::Canon()
{
	char* pChar;
	char ch;
	User& u = Kernel::Instance().GetUser();

	X86Assembly::CLI();
	while ( this->t_delct == 0 )
	{
		if ( (this->t_state & TTy::CARR_ON) == 0 )
		{
			/* �豸û�д򿪣����� */
			return 0;
		}
		u.u_procp->Sleep((unsigned long)&this->t_rawq, ProcessManager::TTIPRI);
	}
	X86Assembly::STI();

loop:
	pChar = &Canonb[2];
	while ( (ch = this->t_rawq.GetChar()) >= 0 )
	{
		if ( 0x7 == ch )	/* 0x07��ÿ�����봮�ķָ��� */
		{
			this->t_delct--;
			break;
		}

		if ( (this->t_flags & TTy::RAW) == 0 )	 /* �������rawģʽ����Ҫ������ת���ַ�ʱ����Ӧ�ı任 */
		{
			if ( pChar[-1] != '\\' )
			{
				if ( ch == this->t_erase ) /* �����backspace��*/
				{
					if ( pChar > &Canonb[2] )	/* ֱ��ɾ��ǰ����ַ� */
					{
						pChar--;
					}
					continue;
				}

				if ( ch == this->t_kill )	/* ��ɾ��һ�� */
				{
					/* ���������¿�ʼ������ʹ��loop��Ȼ�ƻ��˳���ṹ������ȷ�Ǽ������������v6ԭ����������д�� */
					goto loop;
				}

				if ( ch == TTy::CEOT )	/* CEOT == 0x4 */
				{
					/* �����ļ��������������κβ�����������ڽ��������ת��ʱ���ļ������ն˶�дʱΪ�˱���һ�� */
					continue;
				}
			}
		}
		else
		{

		}

		*pChar++ = ch;	/* ��ͨ�ַ�ֱ�ӷ��빤������ */
		if ( pChar >= Canonb + TTy::CANBSIZ )
		{
			break;
		}
	}

	char* pEnd = pChar;
	pChar = &Canonb[2];

	/* ��cook�����ַ��͵��й��򻺴棬��ttread��ȡ */
	while ( pChar < pEnd )
	{
		this->t_canq.PutChar(*pChar++);
	}

	return 1;
}

int TTy::PassC(char ch)
{
	User& u = Kernel::Instance().GetUser();

	/* ���ַ������û�Ŀ���� */
	if ( u.u_IOParam.m_Count > 0 )
	{
		*(u.u_IOParam.m_Base++) = ch;
			u.u_IOParam.m_Count--;
		return 0;
	}
	return -1;
}

char TTy::CPass()
{
	char ch;
	User& u = Kernel::Instance().GetUser();

	ch = *(u.u_IOParam.m_Base++);
	if ( u.u_IOParam.m_Count > 0 )
	{
		u.u_IOParam.m_Count--;
		//u.u_IOParam.m_Offset++;
		return ch;
	}
	else
	{
		return -1;
	}
}


