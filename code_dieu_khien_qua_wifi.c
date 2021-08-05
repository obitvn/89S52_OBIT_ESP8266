/*************************************************************
Dieu khien thi bi qua mang Internet dung ESP8266 và 89S52
Code by Vu Thai
SangTaoClub.Net
**************************************************************/
#include <REGX52.H>
#include <string.h>
#define SIZE    8  //Bo dem buffer
#define TBDK    6  //So TB dieu khien
//Khai bao chan giao tiep LCD16x2 4bit
#define LCD_RS  P1_6
#define LCD_RW  P1_5
#define LCD_EN  P1_4
#define LCD_D4  P1_3
#define LCD_D5  P1_2
#define LCD_D6  P1_1
#define LCD_D7  P1_0
//Khai bao cac thiet bi ngoai vi
#define TB1		P1_7
#define TB2		P2_7
#define TB3		P1_2
#define TB4		P1_3
#define TB5		P1_6
#define TB6		P1_7
#define WRST	P3_7
#define STT		P0_4
#define LINK	P0_5
#define DQ 		P2_4
//Khai bao cac bien, bo dem va mang du lieu
unsigned char u_data, m, k, uptime, tm, tn;
unsigned char stt_ok, send_ok, ok_200, get, linked, http_buffer[75], str_len[5];
unsigned char buffer[SIZE], control[TBDK];
unsigned char temp, err, time_1;
//Chuong trinh tao tre delay
void delay_us(unsigned int t){
        while(t--);
}
void delay_ms(unsigned int t){
	unsigned int i;
	unsigned char j;
	for(i=0;i<t;i++)
		for(j=0;j<140;j++);
}
/****************Ctr giao tiep IC DS18B20***************************/
void delay_18B20(unsigned int ti){
    while(ti--);
}
void Init_18B20(void){
    DQ = 1;   
    delay_18B20(15);
    DQ = 0;   
    delay_18B20(75);
    DQ = 1;   
    delay_18B20(25);    
}
unsigned char ReadOneChar(void){
    unsigned char i=0;
    unsigned char dat = 0;
    for (i=8;i>0;i--){
          DQ = 0;
          dat>>=1;
          DQ = 1;
          if(DQ)
          dat |= 0x80;
          delay_18B20(5);
    }
    return(dat);
}
void WriteOneChar(unsigned char dat){
    unsigned char i=0;
    for (i=8; i>0; i--){
        DQ = 0;
        DQ = dat&0x01;
        delay_18B20(5);
        DQ = 1;
        dat>>=1;
    }
}
void ReadTemperature(void){	
    unsigned char a=0;
    unsigned char b=0;
	EA=0;//Cam ngat tranh viec anh huong qua trinh doc nhiet do
    Init_18B20();
    WriteOneChar(0xCC);        // Bo qua ROM
    WriteOneChar(0x44);
    delay_18B20(15); 
    Init_18B20();
    WriteOneChar(0xCC);
    WriteOneChar(0xBE);    //Doc nhiet do
    delay_18B20(15);
    a=ReadOneChar();        //Read temp low value
    b=ReadOneChar();          //Read temp high value
    temp=((b*256+a)>>4);    //gia tri nhiet do luu vao bien temp
	temp=temp-2;
	EA=1; //Doc xong thi cho phep ngat
}//End code DS18B20
/**************Ctr giao tiep LCD 16x2 4bit**********************/
void LCD_Enable(void){
        LCD_EN =1;
        delay_us(5);
        LCD_EN=0;
        delay_us(60); 
}
void LCD_Send4Bit(unsigned char Data){
        LCD_D4=Data & 0x01;
        LCD_D5=(Data>>1)&1;
        LCD_D6=(Data>>2)&1;
        LCD_D7=(Data>>3)&1;
}
void LCD_SendCommand(unsigned char command){
        LCD_Send4Bit(command >>4);
        LCD_Enable();
        LCD_Send4Bit(command);
        LCD_Enable();
}
void lcd_clear(){
        LCD_SendCommand(0x01); 
        delay_ms(10);
}
void lcd_init(){
        LCD_Send4Bit(0x00);
        delay_ms(20);
        LCD_RS=0;
        LCD_RW=0;
        LCD_Send4Bit(0x03);
        LCD_Enable();
        delay_ms(5);
        LCD_Enable();
        delay_us(110);
        LCD_Enable();
        LCD_Send4Bit(0x02);
        LCD_Enable();
        LCD_SendCommand( 0x28 );
        LCD_SendCommand( 0x0c); 
        LCD_SendCommand( 0x06 );
        LCD_SendCommand(0x01);
}
void lcd_gotoxy(unsigned char x, unsigned char y){
        unsigned char address;
        if(!y)address=(0x80+x);
        else address=(0xc0+x);
        delay_us(1000);
        LCD_SendCommand(address);
        delay_us(50);
}
void lcd_putchar(unsigned char Data){//Ham Gui 1 Ki Tu
        LCD_RS=1;
        LCD_SendCommand(Data);
        LCD_RS=0 ;
}
void lcd_puts(unsigned char *s){//Ham gui 1 chuoi ky tu
        while (*s){
                lcd_putchar(*s);
                s++;
        }
}
void num_str(unsigned char num){
   	 if(num>99){
	 	str_len[0]=(num/100)+48;
	 	str_len[1]=(num%100/10)+48;
	 	str_len[2]=(num%10)+48;
	 }else if(num>9){
	 	str_len[0]=(num/10)+48;
	 	str_len[1]=(num%10)+48;
	 	str_len[2]=0;
	 }else{
	 	str_len[0]=num+48;
	 	str_len[1]=0;
	 	str_len[2]=0;
	}
}

void send_byte(unsigned char udata){
    if(udata){
		SBUF=udata;
		while(!TI); TI=0;
	}
}
void send_r(unsigned char *u){
    while(*u)send_byte(*u++);    
}
void send_buf(unsigned char *u, unsigned char ulen){
    unsigned char z;
    for(z=0;z<ulen;z++)send_byte(*u++);    
}
void send_f(unsigned char code *u){
    while(*u)send_byte(*u++);  
}
void clear(void){
    unsigned char z;
    for(z=0;z<SIZE;z++)buffer[z]='\0';
}
unsigned char len_http(unsigned char len, unsigned char code *d){
    while(*d){
		http_buffer[len]= (*d++);
        len++;
    }
    return len;
}
unsigned char len_http_r(unsigned char len, unsigned char *d){
    while(*d){
		http_buffer[len]= (*d++);
        len++;
    }
    return len;
}	
void wlan_init(){
	unsigned char e=10;
    WRST=0;
	delay_ms(100);
	WRST=1;
    delay_ms(4000);
    stt_ok=0;	
	lcd_gotoxy(0,1);
	ES=1; EA=1;
	while(e--){
	   send_f("AT\r\n"); 
	   delay_ms(1000);
	   if(stt_ok==1){ lcd_puts("WLAN OK!"); break; }
	   if(!e){
			lcd_puts("WLAN ERROR!");
			TR1=0; ES=0; EA=0; time_1=0;
			while(1){
				STT=~STT;
				delay_ms(750);
				if(++time_1>120){ TR1=1; ES=1; EA=1; time_1=1; break; }
			} //CPU stop
	   }
	}
    send_f("AT+CWJAP=\"Obit team\",\"tesla-obit_team\"\r\n"); //Ket noi vao wifi nha ban
    delay_ms(2000);
	//
	delay_ms(2000); 
}
unsigned char getkey(unsigned char key){
	ES=0; //Cam ngat uart
	while(!RI);		
	if(key==SBUF){ RI=0; ES=1; return 1; }
	else{ RI=0; ES=1; return 0; }	
}
void ket_noi(){
    unsigned char do_dai, f=250;
	LINK=1;
	lcd_clear(); lcd_puts("TCP Connecting..");    
	if(P1_5){
		lcd_gotoxy(0,1);
		lcd_puts(">TO SERVER");
		send_f("AT+CIPSTART=\"TCP\",\"sangtaoclub.net\",80\r\n");
	}else{
		lcd_gotoxy(0,1);
		lcd_puts(">LOCALHOST");
		send_f("AT+CIPSTART=\"TCP\",\"192.168.0.107\",80\r\n");
	}
    linked=0; 
    while(f--){
		if(!f){
			lcd_clear(); lcd_puts("No Connect!");
			LINK=1; delay_ms(10000);
			uptime=150; err++; goto quit;
		}
		if(linked==1)break;
		delay_ms(40);
	}
	LINK=0;
	lcd_clear();lcd_puts("Connect OK!"); 
	ReadTemperature(); 
    do_dai=len_http(0,"GET /g.php?t=");
    num_str(temp);
    do_dai=len_http_r(do_dai,str_len);
	lcd_gotoxy(0,1); lcd_puts("Temp: "); lcd_puts(str_len); lcd_puts(" C");	//Show temp
    do_dai=len_http(do_dai," HTTP/1.0\r\n");    
    if(P1_5)do_dai=len_http(do_dai,"Host: dktb.sangtaoclub.net\r\n\r\n"); //server
	else do_dai=len_http(do_dai,"Host: 192.168.0.107\r\n\r\n");  //localhost
	num_str(do_dai); LINK=1;
    send_f("AT+CIPSEND="); send_r(str_len); send_f("\r\n");	 //Send
	while(!getkey('>'));	//Check key >
	LINK=0; lcd_clear(); lcd_puts("Send HTTP...");
    send_buf(http_buffer,do_dai);
	LINK=1; send_ok=0; f=255;
	while(f--){
		if(!f){
			lcd_clear(); lcd_puts("SEND Error!");
			delay_ms(10000);
			uptime=150; err=4; goto quit;
		}
		if(send_ok==1)break;
		delay_ms(80);
	}
	lcd_gotoxy(9,0);
	lcd_puts(" OK!");
	lcd_gotoxy(0,1);
	lcd_puts("Sent: ");
	lcd_puts(str_len);
	lcd_puts(" Byte");
	ok_200=0; get=0; f=255;	
	LINK=0;
	delay_ms(500);
	lcd_gotoxy(0,0);
	lcd_puts("Waiting Data...");
	LINK=1;
    while(f--){
		if(!f){	LINK=1;
			lcd_clear(); lcd_puts("HTTP Error!");
			delay_ms(10000);
			uptime=150; err=4; goto quit;
		}
		if(ok_200)break;
		delay_ms(100);
	}	LINK=0;
	lcd_clear(); lcd_puts("HTTP 200 OK!");
	if(get==1){
		delay_ms(500);
		lcd_gotoxy(0,1);
		lcd_puts("Control...");
	}
	err=0;	
	quit: delay_ms(10);
}    
void xuly(){
    unsigned char tinh;
    if(buffer[0]=='a'){	get=1;	//GET OK
		tinh = buffer[1]-48;  //lay addr
        if(tinh<TBDK)control[tinh]=buffer[3]-48;  //save		
	}else{
        if(strcmp(buffer,"OK")==0)stt_ok=1; //OK
        else if(strcmp(buffer,"Linked")==0)linked=1; //LINK	&  ALREAY CONNECT
        else if(strcmp(buffer,"ok")==0)ok_200=1; //ok 200
		else if(strcmp(buffer,"SEND OK")==0)send_ok=1;
		else if(strcmp(buffer,"ALREAY CONNECT")==0)linked=1;
    }
}
																	     
void uart()interrupt 4 {   
    if(RI){
		u_data=SBUF;
		if((u_data==10 || u_data=='%') && m==0){m=1; clear(); k=0;} //mo phong su dung ky tu dau la %, ky tu cuoi la Enter
    	//if(u_data==10 && m==0){m=1; clear(); k=0;}
    	else if(u_data==13 && m>0){xuly(); m=0;}
    	else if(m>0){
        	buffer[k]=u_data;
        	if(++k>SIZE){m=0; clear();}
    	}	
		RI=0;
	}	 
}
void prog_control(){
    if(control[0]>0){ TB1=1; }else{ TB1=0; }
    if(control[1]>0){ TB2=1; }else{ TB2=0; }
    if(control[2]>0){ TB3=1; }else{ TB3=0; }
    if(control[3]>0){ TB4=1; }else{ TB4=0; }
	if(control[4]>0){ TB5=1; }else{ TB5=0; }
    if(control[5]>0){ TB6=1; }else{ TB6=0; }
}	
void main(void){ //Chuong trinh chinh
    //unsigned char ab;
	P1=0x30; LINK=1; STT=1; WRST=1;
    TMOD=0x20; SCON=0x50; TH1=TL1=0xFD; TR1=1; //UART init
    lcd_init();	//Khoi tao lcd
	delay_ms(500);
	lcd_puts("DO AN TOT NGHIEP");
	STT=0; delay_ms(500);	
	lcd_gotoxy(0,1); lcd_puts("Staring...");
	start: for(time_1=0;time_1<4;time_1++){ STT=1; delay_ms(200); STT=0; delay_ms(300); }
	/*** Thiet lap WLAN ***/
	lcd_clear(); lcd_puts("WLAN Init..."); 
    wlan_init();
	if(time_1==1){ time_1=0; goto start; }
	/*** Thiet lap nhiet do ***/
	lcd_clear(); lcd_puts("Temperature Init");
	ReadTemperature();
	while(temp>60 || temp==0){	//Check temp error
		delay_ms(1000);
		ReadTemperature();
		if(++tn>4){
			lcd_clear(); lcd_puts("Temperature");
			lcd_gotoxy(0,1); lcd_puts("Sensor Error!");
			ES=0; EA=0; TR1=0; time_1=0;
			while(1){
				STT=~STT;
				delay_ms(500);
				if(++time_1>130){ TR1=1; ES=1; EA=1; time_1=1; break; }
			} //CPU stop
			if(time_1==1)break;
		}	
	}
	if(time_1==1){ time_1=0; goto start; }
	lcd_gotoxy(12,0); lcd_puts(" OK!");	
	lcd_gotoxy(0,1); lcd_puts("Temp: ");
	num_str(temp); lcd_puts(str_len); lcd_puts(" C");
	delay_ms(1000);	uptime=150;  	
    while(1){
		if(++uptime>120){  //Ket noi may chu
			uptime=0;
			ket_noi();
        	prog_control(); //Dieu khien ngoai vi	
		}
		if(err>2){err=0; goto start; } //Auto Reset	
        delay_ms(100);  //Tre he thong	
    }
}