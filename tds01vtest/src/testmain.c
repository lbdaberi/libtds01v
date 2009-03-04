/*
 ============================================================================
 Name        : tds01vtest.c
 Author      : lb
 Version     :
 Copyright   : Your copyright notice
 Description : libtds01vのテストサンプル
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libtds01v.h"

/*
 *コールバック関数
 *計測値を出力
 */
void getInfo(tds_event_t* event) {
	printf("mag_x=[%f]\n", event->mag_x);
	printf("mag_y=[%f]\n", event->mag_y);
	printf("mag_z=[%f]\n", event->mag_z);
	printf("azimuth=[%f]\n", event->azimuth);
	printf("acc_x=[%f]\n", event->acc_x);
	printf("acc_y=[%f]\n", event->acc_y);
	printf("acc_z=[%f]\n", event->acc_z);
	printf("roll=[%f]\n", event->roll);
	printf("pitch=[%f]\n", event->pitch);
	printf("air_pressure=[%f]\n", event->air_pressure);
	printf("altitude=[%d]\n", event->altitude);
	printf("temperature=[%f]\n", event->temperature);
	printf("voltage=[%d]\n", event->voltage);
}

int main(void) {

	/*ライブラリのハンドラ*/
	int handler;

	/*libtds01vオープン*/
	handler = opentds();
	/*コールバック関数登録*/
	addCallback(getInfo);
	/*計測開始*/
	starttds(handler);

	sleep(10);
	/*計測終了*/
	stoptds(handler);
	/*libtds01vクローズ*/
	closetds(handler);

	return 0;
}
