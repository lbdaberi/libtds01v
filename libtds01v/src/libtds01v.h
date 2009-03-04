/*
 ============================================================================
 Name        : libtds01v.h
 Author      : lb
 Version     :
 Copyright   : Your copyright notice
 Description : libtds01vのインクルードファイル
 ============================================================================
 */

#ifndef LIBTDS01V_H_
#define LIBTDS01V_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>

/*
 * TDS01Vから取得した情報を格納したイベント構造体
 */
typedef struct tds_event{
	/*地磁気(μT) x方向*/
	float mag_x;
	/*地磁気(μT) y方向*/
	float mag_y;
	/*地磁気(μT) z方向*/
	float mag_z;
	/*方位角(度)*/
	float azimuth;
	/*加速度(G) x方向*/
	float acc_x;
	/*加速度(G) y方向*/
	float acc_y;
	/*加速度(G) z方向*/
	float acc_z;
	/*傾斜角(度)　ロール*/
	float roll;
	/*傾斜角(度)　ピッチ*/
	float pitch;
	/*気圧(hPa)*/
	float air_pressure;
	/*高度(m)*/
	int altitude;
	/*チップ内部温度(度)*/
	float temperature;
	/*アナログ供給電圧(mV)*/
	int voltage;
}tds_event_t;

/*
 * libtds01vのオープン処理
 * 初期化、リセット要求を行う
 */
int opentds(void);
/*
 * 計測を開始する
 */
int starttds(int);
/*
 * 計測を中止する
 */
void stoptds(int);
/*
 * ライブラリのクローズ処理
 */
void closetds(int);
/*
 * イベント情報通知コールバック関数の登録
 */
bool addCallback(void (*callback)(tds_event_t*));
/*
 * 登録済みコールバック関数の削除
 */
void removeCallback(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIBTDS01V_H_ */
