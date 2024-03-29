#include "function.h"


  extern void uip_polling(void);
	extern uint8_t set_data[22];
	extern uint8_t USART_RX_BUF[25]; //DMA接收数据缓存区
	extern uint8_t USART_RX_BUF1[6];
	extern uint8_t EN[21];           //使能拨码数组
  extern uint8_t rx_len;           //数据长度
	extern uint8_t CH_NUM_temp ;
 	uint8_t receive_data[21][50];    //做解析用的数组，用来存放DMA直接接收到的数据，receive_data[0][25]为无效数据
	uint8_t receive_signal_data[21][6];
  uint8_t  cnt;										 //计数变量
	uint8_t CH_NUM = 0x0B;
	uint8_t slave_id_max = 0x0A;
	uint8_t receive_slave_data[21][26];//传送到PC端和DP的数据�
	uint8_t receive_slave_signal_data[21];
	uint8_t data_to_dp_1[2];
	uint8_t data_to_dp_2[2];
	uint8_t dp_offline_flag[21];
	uint8_t data_to_dp_temp[21][16];
	uint8_t offline_to_tcp[21];
	uint8_t tcp_server_tsta=0XFF;
	uint8_t signal_intensity[5];
//	uint8_t signal_intensity_flag = 0;
	uint8_t signal_data[21][6];
	/*****************offline_flag[i]=1   为通信正常           绿灯 
										offline_flag[i] = 2 掉线和不使能         红灯
										offline_flag[i] = 3 通信出现帧错误       蓝灯
										其中offline_flag[0] 没用到从1以后就是对应的从站
	****************/


	
	uint8_t offline_flag[21] ;              //数据标志位offline_flag[0]为无效数据
	uint8_t offline_cnt[21] = {0};         //标志位计数数组
	uint8_t offline_flag_temp[21] = {0};   //标志位的中间变量

	int test_get;
	
	uint8_t slave_id ;//从站zigbee ID号
	
	uint8_t slave_id_cnt;
		
  int a=0,b=0,c,d=2,i;

  uint8_t data_header[] = {0x55,0x55,0x00};//接收数据帧头

  uint8_t data_end[] = {0x0d,0x0d};//接收数据帧尾

	uint32_t checksum = 0;//校验和
 
	uint16_t frame_to_slave[5] = {0x77,0x77,0x00,0x66,0x66};//向从站发送要数据指令,帧头为0x77 0x77,第三位为从站ID，帧尾为0x66 0x66
//	
//	uint8_t data_header_1[] = {0x11,0x11};//接收数据帧头

//  uint8_t data_end_1[] = {0x22,0x22};//接收数据帧尾

/*--------------------------------------------------------------------
函数名称：unsigned char SumCheck(unsigned char data) 
函数功能：做校验和用
注意事项：
提示说明：
输    入：
返    回：
--------------------------------------------------------------------*/
	

unsigned char SumCheck(unsigned char data)		
{
	return ((data) / 16 + data % 16);
}


/*--------------------------------------------------------------------
函数名称：uint8_t Get_SumCheck(uint8_t x,uint8_t array[x][25]) 
函数功能：把一个二维数组的x行的数据求和，只求2-20
注意事项：
提示说明：
输    入：
返    回：
--------------------------------------------------------------------*/
	
uint8_t Get_SumCheck(uint8_t x,uint8_t array[x][26])
{
	uint8_t i,sum=array[x][2];
	for(i=4;i<22;i++)
	{
		sum += SumCheck(array[x][i]);
	}
	if(x>15)
	{
		sum=sum-0x0f; 
	}
	return sum;
}	
  
/*--------------------------------------------------------------------
函数名称：void scan_enable_receive(void)
函数功能：扫描使能，并解析数据
注意事项：
提示说明：
输    入：
返    回：
--------------------------------------------------------------------*/
 
void scan_enable_receive(void)
{


for(slave_id=1;slave_id<21;slave_id++) //轮询20个从站
{

//	if(EN[slave_id]==1 || slave_id == 10 || slave_id == 20)                  //判断使能拨码是否使能，1：使能；0：失能
//	{
			
		frame_to_slave[2] = slave_id;         //向从站发送的指令的第三个数组元素是从站ID
	  data_header[2] = slave_id;            //帧头的第三个数据为从站ID

	  if(EN[slave_id]==1)
		{
				UartASendStr_slow(frame_to_slave,5);  //连续向从站发送10次要数指令
	  }
		
	

		delay_ms(100);                        //延时不可改，150合适

		
	
		DataParser *data_parser = parser_init(data_header, sizeof(data_header), data_end, sizeof(data_end), 25);//初始化一针数据
		

		for(i=0;i<25;i++)
		{
			
			receive_data[slave_id][i] = USART_RX_BUF[i];//将DMA的数据拷贝到receive_data的二维数组中
			
		}
		
		
		memset(USART_RX_BUF,0,sizeof(USART_RX_BUF));                   //将USART_RX_BUF中的数据清零
		
		delay_ms(5); //100ms 开始的时候

		
		for(i = 0; i<25; i++)                        //在receive_data中的数据解析for(i = 0; i<sizeof(receive_data); i++)
		{
			if(parser_put_data (data_parser, receive_data[slave_id][i]) == RESULT_TRUE )//解析成功
			{
					
				  memset(receive_data[slave_id],0,sizeof(receive_data[slave_id])); //对应数组清零
				

				
			
				for(cnt = 4;cnt<24;cnt++)
				{
					receive_slave_data[slave_id][cnt] = parser_get_data(data_parser, cnt-4);//取出解析成功的中间的数
			
				}
				for(u8 v=0;v<16;v++)
				{
					data_to_dp_temp[slave_id][v] = receive_slave_data[slave_id][v+6];
				}
			
				
		  
				if(parser_get_data(data_parser, 19) == slave_id  )//做进一步判断，是否接收到的第19个数是从站ID
				{		
					offline_cnt[slave_id] = 0;                      //将offline_cnt数组清零
					checksum = Get_SumCheck(slave_id,receive_slave_data);//计算解析成功的数的校验和
				  test_get = parser_get_data(data_parser, 18);//取出接收到的第18个数为数据的校验和
					if(test_get == checksum)
					{
						offline_flag[slave_id] = 1;                  //如果接收到的数据的校验和和计算的校验和一致，数据正确，置标志位
						dp_offline_flag[slave_id] = 1;
						offline_to_tcp[slave_id] = 1;
					}
				}
				else
				{
					offline_cnt[slave_id]++;                //如果和从站ID不相等，掉线计数标志位++
				}	
				
			
			}
			  
			else
			{
				offline_flag_temp[slave_id] = 1;        //掉线标志位中间变量
				
			}

		}

		      
		
		
//			uip_polling();                            //网络的传送初始化等
		
		if (slave_id == 10)
	 {
			data_to_dp_1[0] = 0x00 | (dp_offline_flag[1]<< 1) | dp_offline_flag[2];
		 
			data_to_dp_1[1] = (dp_offline_flag[3]<< 7)| (dp_offline_flag[4]<< 6)| (dp_offline_flag[5]<< 5)| 
		   (dp_offline_flag[6]<< 4)| (dp_offline_flag[7]<< 3)| (dp_offline_flag[8]<< 2)| (dp_offline_flag[9]<< 1)
		 | dp_offline_flag[10];
		 
		  Uart2ASendStr_slow (data_to_dp_1,2); 
		 for(u8 num1=1;num1<11;num1++)
		 {
				Uart2ASendStr_slow (data_to_dp_temp[num1],16); 
		 }
	 }
	 
	 
	    if (slave_id == 20)
	 {
			data_to_dp_2[0] = 0x80 | (dp_offline_flag[11]<< 1) | dp_offline_flag[12];
		 
			data_to_dp_2[1] = (dp_offline_flag[13]<< 7)| (dp_offline_flag[14]<< 6)| (dp_offline_flag[15]<< 5)| 
		   (dp_offline_flag[16]<< 4)| (dp_offline_flag[17]<< 3)| (dp_offline_flag[18]<< 2)| (dp_offline_flag[19]<< 1)
		 | dp_offline_flag[20];
		 
		  Uart2ASendStr_slow (data_to_dp_2,2); 
		 for(u8 num1=11;num1<21;num1++)
		 {
				Uart2ASendStr_slow (data_to_dp_temp[num1],16); 
		 }
	 }

		
		parser_release(data_parser);   //释放解析器，一定用完要释放
		if(offline_flag_temp[slave_id] == 1) //判断是否有接受到正确的数据
	  {
			offline_flag_temp[1] = 0;
			offline_cnt[slave_id]++;
			if(offline_cnt[slave_id]>10 && offline_cnt[slave_id]<=15)//5-10次表示帧错误
			{
				offline_flag[slave_id] = 3;
				offline_to_tcp[slave_id] = 1;
			}
			if(offline_cnt[slave_id]>15)//大于10次表示掉线
			{
				offline_flag[slave_id] = 2;
				dp_offline_flag[slave_id] = 0;
				offline_to_tcp[slave_id] = 2;
			}
			
	  }

			
//  }	
	
	
	
if(EN[slave_id]==0)
	{
		offline_flag[slave_id] = 4;//失能灭灯
		dp_offline_flag[slave_id] = 0;
		offline_to_tcp[slave_id] = 3;
		for(u8 x=4;x<24;x++)
		{
			receive_slave_data[slave_id][x] = 0x00;
		}
	}

	 receive_slave_data[slave_id][0]=receive_slave_data[slave_id][1]=0x55;//将帧头0x55赋值给前两个数
	 receive_slave_data[slave_id][2]=slave_id;                            //将从站ID赋值给第三个数
	 receive_slave_data[slave_id][3]=offline_to_tcp[slave_id];
	 receive_slave_data[slave_id][24]=receive_slave_data[slave_id][25]=0x0d;//将帧头赋值给最后两个数
	

  uip_polling();                   //网络的传送初始化等
	

	MYDMA_Enable(DMA1_Channel5);//使能DMA
	
	
	IWDG_Feed();
	

		if(offline_flag[slave_id]==2)
		{
			for(u8 x=4;x<24;x++)
		 {
			 receive_slave_data[slave_id][x] = 0x00;
		 }
		}
	
}
}


