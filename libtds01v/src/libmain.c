/*
 ============================================================================
 Name        : libmain.c
 Author      : lb
 Version     :
 Copyright   : Your copyright notice
 Description : libtds01v本体
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include "libtds01v.h"

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyUSB0"
#define _POSIX_SOURCE 1
#define ACK	true
#define NAK	false

typedef struct termios termios_t;

/*ライブラリが終了するか*/
volatile bool is_stop = true;
/*計測スレッドが終了したか*/
volatile bool is_finalize_end = false;
termios_t oldtio;
/*コールバック関数*/
void (*func)(tds_event_t*) = NULL;

/*
 * ライブラリの初期化
 * シリアルデバイスをオープンし、通信に必要な設定を行う。
 */
int tds_init(int fd, termios_t* newtio) {
	if(0 > (fd = open(MODEMDEVICE, O_RDWR|O_NOCTTY))) {
		perror(MODEMDEVICE);
		return -1;
	}

	tcgetattr(fd, &oldtio);
	memset(newtio, 0, sizeof(termios_t));

	newtio->c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio->c_iflag = IGNPAR | ICRNL;
	newtio->c_oflag = 0;
	newtio->c_lflag = ICANON;

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, newtio);

	return fd;
}

/*
 * TDS01Vへのリセットの送信。
 */
bool tds_send_reset(int fd) {
	char buf[255];
	char tmp_buf[2];
	int size = 0;
	bool ret = NAK;

	/*リセットコマンド送信*/
	write(fd, "0F\r\n", 4);
	/*応答の受信*/
	size = read(fd, buf, sizeof(buf));
	/*応答は改行コードが2回来るので二つ目の改行コードを読み込んでおく*/
	read(fd, tmp_buf, sizeof(tmp_buf));

	if(0 < size) {
		/*応答がACKであるか*/
		if('F' == buf[0] && '0' == buf[1]) {
			ret = ACK;
		}
	}

	/*動作がアイドルになるまでセンサデバイスの状態を要求する*/
	do {
		memset(buf, 0, sizeof(buf));
		write(fd, "2B\r\n", 4);
		size = read(fd, buf, sizeof(buf));
		read(fd, tmp_buf, sizeof(tmp_buf));
	} while('0' != buf[0] && '0' != buf[1]);

	return ret;
}

/*
 * 受信で読み込んだバッファを指定indexからoffset分の文字列にする
 */
void tds_parse_buffer(const char* buf, char parsed_string[], int index, int offset) {
	memset(parsed_string, 0, offset+1);
	memcpy(parsed_string, &buf[index], offset);
}

/*
 * 磁気センサベクトルデータを受信データから取り出す
 */
void tds_parse_mag(const char* buf, float* mag) {
	unsigned int u;
	int16_t i;
	sscanf(buf, "%X", &u);
	i = (int16_t)u;
	*mag = (float)i / 10.0;
}

/*
 * 方位角を受信データから取り出す
 */
void tds_parse_azimuth(const char* buf, float* azimuth) {
	tds_parse_mag(buf, azimuth);
}

/*
 * 加速度センサベクトルデータを受信データから取り出す
 */
void tds_parse_acc(const char* buf, float* acc) {
	unsigned int u;
	int16_t i;
	sscanf(buf, "%X", &u);
	i = (int16_t)u;
	*acc = (float)i / 100.00;
}

/*
 * 傾斜角を受信データから取り出す
 */
void tds_parse_angle(const char* buf, float* angle) {
	tds_parse_mag(buf, angle);
}

/*
 * 気圧を受信データから取り出す
 */
void tds_parse_pressure(const char* buf, float* pressure) {
	tds_parse_mag(buf, pressure);
}

/*
 * 高度を受信データから取り出す
 */
void tds_parse_alititude(const char* buf, int* altitude) {
	unsigned int u;
	int16_t i;
	sscanf(buf, "%X", &u);
	i = (int16_t)u;
	*altitude = i;
}

/*
 * チップ内部温度を受信データから取り出す
 */
void tds_parse_temperature(const char* buf, float* temperature) {
	tds_parse_mag(buf, temperature);
}

/*
 * アナログ供給電圧を受信データから取り出す
 */
void tds_parse_voltage(const char* buf, int* voltage) {
	unsigned int u;
	int16_t i;
	sscanf(buf, "%X", &u);
	i = (int16_t)u;
	*voltage = i;
}

/*
 * 受信データから各情報を取得し、イベント構造体に格納する
 */
void tds_parse_info(const char* buf, tds_event_t* event) {
	char parsed_str[255];

	memset(parsed_str, 0, sizeof(parsed_str));
	/*mag_x*/
	tds_parse_buffer(buf, parsed_str, 0, 4);
	tds_parse_mag(parsed_str, &(event->mag_x));
	/*mag_y*/
	tds_parse_buffer(buf, parsed_str, 4, 4);
	tds_parse_mag(parsed_str, &(event->mag_y));
	/*mag_z*/
	tds_parse_buffer(buf, parsed_str, 8, 4);
	tds_parse_mag(parsed_str, &(event->mag_z));
	/*azimuth*/
	tds_parse_buffer(buf, parsed_str, 12, 4);
	tds_parse_azimuth(parsed_str, &(event->azimuth));
	/*acc_x*/
	tds_parse_buffer(buf, parsed_str, 16, 4);
	tds_parse_acc(parsed_str, &(event->acc_x));
	/*acc_y*/
	tds_parse_buffer(buf, parsed_str, 20, 4);
	tds_parse_acc(parsed_str, &(event->acc_y));
	/*acc_z*/
	tds_parse_buffer(buf, parsed_str, 24, 4);
	tds_parse_acc(parsed_str, &(event->acc_z));
	/*roll*/
	tds_parse_buffer(buf, parsed_str, 28, 4);
	tds_parse_angle(parsed_str, &(event->roll));
	/*pitch*/
	tds_parse_buffer(buf, parsed_str, 32, 4);
	tds_parse_angle(parsed_str, &(event->pitch));
	/*pressure*/
	tds_parse_buffer(buf, parsed_str, 36, 4);
	tds_parse_pressure(parsed_str, &(event->air_pressure));
	/*altitude*/
	tds_parse_buffer(buf, parsed_str, 40, 4);
	tds_parse_alititude(parsed_str, &(event->altitude));
	/*temperature*/
	tds_parse_buffer(buf, parsed_str, 44, 4);
	tds_parse_temperature(parsed_str, &(event->temperature));
	/*voltage*/
	tds_parse_buffer(buf, parsed_str, 48, 4);
	tds_parse_voltage(parsed_str, &(event->voltage));
}

/*
 * 計測スレッドのメイン処理
 * stoptdsが呼ばれるまで計測データを取得し続ける
 */
void* tds_watchdog_main(void* args) {
	int fd = *((int*)args);
	char buf[255];
	char tmp_buf[2];
	int size = 0;
	tds_event_t event;

	memset(buf, 0, sizeof(buf));
	/*計測条件設定 TODO:偏角設定*/
	//偏角近似式(7+(37.142/60)+(21.622/60)*(a+(b+c/60)/60-37)-(7.672/60)*(p+(q+r/60)/60-138)-(0.422/60)*(a+(b+c/60)/60-37)^2-(0.320/60)*(a+(b+c/60)/60-37)*(p+(q+r/60)/60-138)-(0.675/60)*(p+(q+r/60)/60-138)^2);
	write(fd, "050027950000\r\n", 14);
	size = read(fd, buf, sizeof(buf));
	read(fd, tmp_buf, sizeof(tmp_buf));

	/*センサ情報項目設定　全データ*/
	write(fd, "0DF7\r\n", 6);
	size = read(fd, buf, sizeof(buf));
	read(fd, tmp_buf, sizeof(tmp_buf));

	/*地磁気センサ初期化*/
	write(fd, "27\r\n", 4);
	size = read(fd, buf, sizeof(buf));
	read(fd, tmp_buf, sizeof(tmp_buf));

	while(false == is_stop) {
		memset(buf, 0, sizeof(buf));
		memset(&event, 0, sizeof(event));
		/*計測開始*/
		write(fd, "21\r\n", 4);
		size = read(fd, buf, sizeof(buf));
		read(fd, tmp_buf, sizeof(tmp_buf));

		/*計測開始応答がACKならセンサ情報を要求する*/
		if('D' == buf[0] && 'E' == buf[1]) {
			/*センサ情報要求*/
			write(fd, "29\r\n", 4);
			size = read(fd, buf, sizeof(buf));
			read(fd, tmp_buf, sizeof(tmp_buf));
			/*取得したセンサ情報文字列をイベント構造体に変換*/
			tds_parse_info(buf, &event);

			if(NULL != func) {
				/*コールバック関数呼び出し*/
				func(&event);
			}
		}
	}

	is_finalize_end = true;
	return NULL;
}

/*
 * libtds01vのオープン処理
 * 初期化、リセット要求を行う
 */
int opentds(void) {
	;
	int fd;
	termios_t newtio;
	//	char buf[255];

	if(0 > (fd = tds_init(fd, &newtio))) {
		return -1;
	}

	if(ACK != tds_send_reset(fd)) {
		perror("reset.");
		return -1;
	}

	return fd;
}

/*
 * 計測を開始する
 */
int starttds(int handler) {
	pthread_t wthread;

	/*終了フラグがfalseならすでに計測スレッドが開始しているものとしてエラーを返す*/
	if(false == is_stop) {
		return -1;
	}
	/*計測スレッド開始*/
	if(0 != pthread_create(&wthread, NULL, tds_watchdog_main, &handler)) {
		perror("create thread.");
		return -1;
	}
	is_stop = false;
	return 0;
}

/*
 * 計測を中止する
 */
void stoptds(int handler) {
	is_stop = true;
}

/*
 * ライブラリのクローズ処理
 */
void closetds(int handler) {
	while(false == is_finalize_end) {
		;
	}

	tcsetattr(handler,TCSANOW,&oldtio);
	close(handler);
}

/*
 * イベント情報通知コールバック関数の登録
 * 登録済みの場合は登録せずfalseを返す
 */
bool addCallback(void (*callback)(tds_event_t*)) {
	if(NULL == func) {
		func = callback;
		return true;
	} else {
		return false;
	}
}

/*
 * 登録済みコールバック関数の削除
 */
void removeCallback(void) {
	func = NULL;
}
