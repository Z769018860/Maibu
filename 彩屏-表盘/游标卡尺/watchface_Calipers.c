/*
 * =====================================================================================
 * 
 *     Modulename:
 *       Filename:  watchface_Calipers.c
 *
 *    Description:  游标卡尺表盘
 *    Corporation:
 * 
 *         Author:  claire 
 *        Created:  2016年03月16日
 *
 * =====================================================================================
 *
 * =====================================================================================
 * 
 *   MODIFICATION HISTORY :
 *    
 *		     DATE :
 *		     DESC :
 * =====================================================================================
 */	
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "maibu_sdk.h"
#include "maibu_res.h"

#define BACKGROUND_NUM 1 //背景图片个数
#define LAYER_NUM 3 //需定时更新的图层个数

/*图片坐标：第一项为背景图片，后面依次为步数，卡路里，公里数图层的最后一位*/
static uint8_t ORIGIN_X[LAYER_NUM+BACKGROUND_NUM] = {0, 16, 16, 105};
static uint8_t ORIGIN_Y[LAYER_NUM+BACKGROUND_NUM] = {0, 12, 94, 4};
static uint8_t ORIGIN_W[LAYER_NUM+BACKGROUND_NUM] = {176, 78, 78, 54};
static uint8_t ORIGIN_H[LAYER_NUM+BACKGROUND_NUM] = {176, 70, 70, 156};


/*图片名数组：分别为数字0~9图片*/
uint16_t bmp_array[10] = {PIC_0_1, PIC_1_1, PIC_2_1, PIC_3_1, PIC_4_1, PIC_5_1, PIC_6_1, PIC_7_1, PIC_8_1, PIC_9_1};


/*小时分钟月日图层ID，通过该图层ID获取图层句柄*/
static int8_t g_layer_time[LAYER_NUM] ={ -1, -1, -1};

/*窗口ID, 通过该窗口ID获取窗口句柄*/
static int32_t g_window = -1;

/*表盘应显示数据*/
static uint8_t watch_data[LAYER_NUM] = {0,0,0};

/*
 *--------------------------------------------------------------------------------------
 *     function:  app_watch_data
 *    parameter: 
 *       return:
 *  description:  将数组参数赋值为当前表盘应显示值
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static void app_watch_data()
{
	int8_t percent;
	maibu_get_battery_percent(&percent);

	struct date_time datetime;
	app_service_get_datetime(&datetime);

	watch_data[0] = datetime.hour/10;
	watch_data[1] = datetime.hour%10;
	watch_data[2] = 156-datetime.min/5*14;

}



/*
 *--------------------------------------------------------------------------------------
 *     function:  get_layer
 *    parameter: 
 *       return:
 *  description:  生成表盘窗口的各图层
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static P_Layer get_layer(uint8_t pos)
{
	GRect frame = {{ORIGIN_X[pos], ORIGIN_Y[pos]}, {ORIGIN_H[pos], ORIGIN_W[pos]}};
	GBitmap bitmap;
	P_Layer	 layer_back = NULL;
		
	/*获取用户图片*/
	if (pos < BACKGROUND_NUM)
	{
		res_get_user_bitmap(PIC_BG, &bitmap);
	}else
	if (pos == LAYER_NUM)
	{
		GPoint p1 = {ORIGIN_X[pos], ORIGIN_Y[pos]}, p2 = {ORIGIN_X[pos], ORIGIN_Y[pos]+watch_data[2]},
			p3 = {ORIGIN_X[pos]+ORIGIN_W[pos] , ORIGIN_Y[pos]}, p4 = {ORIGIN_X[pos]+ORIGIN_W[pos] , ORIGIN_Y[pos]+watch_data[2]};

		/*多边形*/
		GPoint points[4] = {p1, p2, p3, p4};
		Polygon po = {4, points};
		Geometry pg = {GeometryTypePolygon, FillArea, GColorBlack, (void*)&po}; 
		P_Geometry p_pg[1];
		LayerGeometry lg;
		memset(&lg, 0, sizeof(LayerGeometry));
		p_pg[lg.num++] = &pg;
		lg.p_g = p_pg;

		/*生成几何图形图层*/
		layer_back = app_layer_create_geometry(&lg);
		return (layer_back);
	}else
	{
		res_get_user_bitmap(bmp_array[watch_data[pos-BACKGROUND_NUM]], &bitmap);
	}
	LayerBitmap layerbmp = { bitmap, frame, GAlignLeft};	

	/*生成图层*/
	layer_back = app_layer_create_bitmap(&layerbmp);

	return (layer_back);
}



/*
 *--------------------------------------------------------------------------------------
 *     function:  app_watch_update
 *    parameter: 
 *       return:
 *  description:  更新图层数据
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static void app_watch_update()
{
	P_Window p_window = NULL;
	P_Layer p_old_layer = NULL, p_new_layer = NULL;
	uint8_t pos;

	/*根据窗口ID获取窗口句柄*/
	p_window = app_window_stack_get_window_by_id(g_window);
	if (p_window == NULL)
	{
		return;
	}

	app_watch_data();

	for (pos=0; pos<LAYER_NUM; pos++)
	{
		/*获取数据图层句柄*/
		p_old_layer = app_window_get_layer_by_id(p_window, g_layer_time[pos]);	
		if (p_old_layer != NULL)
		{
			/*更新数据图层*/
			p_new_layer = get_layer(pos+1);
			app_window_replace_layer(p_window, p_old_layer, p_new_layer);
		}
	}

	/*窗口显示*/	
	app_window_update(p_window);
}


/*
 *--------------------------------------------------------------------------------------
 *     function:  app_watch_time_change 
 *    parameter: 
 *       return:
 *  description:  系统时间有变化时，更新时间图层
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static void app_watch_time_change(enum SysEventType type, void *context)
{

	/*时间更改*/
	if (type == SysEventTypeTimeChange)
	{
		app_watch_update();	
	}
}



/*
 *--------------------------------------------------------------------------------------
 *     function:  init_watch
 *    parameter: 
 *       return:
 *  description:  生成表盘窗口
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static P_Window init_watch(void)
{
	P_Window p_window = NULL;
	P_Layer layer = NULL;
	uint8_t pos;

	/*创建一个窗口*/
	p_window = app_window_create();
	if (NULL == p_window)
	{
		return NULL;
	}

	/*创建背景图层*/
	layer = get_layer(0);

	/*添加背景图层到窗口*/
	app_window_add_layer(p_window, layer);

	app_watch_data();

	/*创建所有图层并添加到窗口*/
	for (pos=BACKGROUND_NUM; pos<LAYER_NUM+BACKGROUND_NUM; pos++)
	{
		layer = get_layer(pos);
		g_layer_time[pos-BACKGROUND_NUM] = app_window_add_layer(p_window, layer);
	}
	

	/*注册一个事件通知回调，当有时间改变时，立即更新时间*/
	maibu_service_sys_event_subscribe(app_watch_time_change);

	return p_window;
}



/*
 *--------------------------------------------------------------------------------------
 *     function:  main
 *    parameter: 
 *       return:
 *  description:  主程序
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
int main()
{
	/*模拟器模拟显示时打开，打包时屏蔽*/
	//simulator_init();

	/*创建显示表盘窗口*/
	P_Window p_window = init_watch();
	if (p_window != NULL)
	{
		/*放入窗口栈显示*/
		g_window = app_window_stack_push(p_window);
	}
	
	/*模拟器模拟显示时打开，打包时屏蔽*/
	//simulator_wait();

	return 0;
}
