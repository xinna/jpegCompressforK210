/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <string.h>
#include "dvp.h"
#include "fpioa.h"
#include "lcd.h"
#include "ov5640.h"
#include "ov2640.h"
#include "gpiohs.h"
#include "plic.h"
#include "sysctl.h"
#include "uarths.h"
#include "nt35310.h"
#include "board_config.h"
#include "jpeg.h"
#include <stdbool.h>

uint32_t g_lcd_gram0[38400] __attribute__((aligned(64)));
uint32_t g_lcd_gram1[38400] __attribute__((aligned(64)));

volatile uint8_t g_dvp_finish_flag;
volatile uint8_t g_ram_mux;



volatile uint32_t g_gpio_flag = 0;
volatile uint32_t g_key_press = 0;
volatile uint64_t g_gpio_time = 0;

volatile uint32_t key1_press_flag = 0;
volatile uint64_t key1_press_time = 0;
volatile uint32_t key2_press_flag = 0;
volatile uint64_t key2_press_time = 0;
int irq_key0(void* ctx)
{
    //printf("Key0 Working\n");
    g_gpio_flag = 1;
    g_gpio_time = sysctl_get_time_us();
    return 0;
}

int irq_key1(void* ctx)
{
//    printf("Key1 Working\n");
    key1_press_flag = 1;
    key1_press_time = sysctl_get_time_us();
    return 0;
}
int irq_key2(void* ctx)
{
//    printf("Key2 Working\n");
    key2_press_flag = 1;
    key2_press_time = sysctl_get_time_us();
    return 0;
}


static int on_irq_dvp(void *ctx)
{
    if (dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        /* switch gram */
        dvp_set_display_addr(g_ram_mux ? (uint32_t)g_lcd_gram0 : (uint32_t)g_lcd_gram1);

        // dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
        g_dvp_finish_flag = 1;
    }
    else
    {
        if (g_dvp_finish_flag == 0)
            dvp_start_convert();
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }

    return 0;
}

/**
 *	dvp_pclk	io47
 *	dvp_xclk	io46
 *	dvp_hsync	io45
 *	dvp_pwdn	io44
 *	dvp_vsync	io43
 *	dvp_rst		io42
 *	dvp_scl		io41
 *	dvp_sda		io40
 * */

/**
 *  lcd_cs	    36
 *  lcd_rst	37
 *  lcd_dc	    38
 *  lcd_wr 	39
 * */

static void io_mux_init(void)
{

#if BOARD_LICHEEDAN
    /* Init DVP IO map and function settings */
    fpioa_set_function(42, FUNC_CMOS_RST);
    fpioa_set_function(44, FUNC_CMOS_PWDN);
    fpioa_set_function(46, FUNC_CMOS_XCLK);
    fpioa_set_function(43, FUNC_CMOS_VSYNC);
    fpioa_set_function(45, FUNC_CMOS_HREF);
    fpioa_set_function(47, FUNC_CMOS_PCLK);
    fpioa_set_function(41, FUNC_SCCB_SCLK);
    fpioa_set_function(40, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(36, FUNC_SPI0_SS3);
    fpioa_set_function(39, FUNC_SPI0_SCLK);


    fpioa_set_function(17, FUNC_GPIOHS8);   //IO17映射到gpio8,key0
    gpiohs_set_drive_mode(8, GPIO_DM_INPUT_PULL_UP);

    gpiohs_set_pin_edge(8, GPIO_PE_FALLING);

    gpiohs_irq_register(8, 1, irq_key0, NULL);

    fpioa_set_function(16, FUNC_GPIOHS9);   //IO16映射到gpio8,key1
    gpiohs_set_drive_mode(9, GPIO_DM_INPUT_PULL_UP);

    gpiohs_set_pin_edge(9, GPIO_PE_FALLING);

    gpiohs_irq_register(9, 1, irq_key1, NULL);

    fpioa_set_function(18, FUNC_GPIOHS10);   //IO16映射到gpio8,key2
    gpiohs_set_drive_mode(10, GPIO_DM_INPUT_PULL_UP);

    gpiohs_set_pin_edge(10, GPIO_PE_FALLING);

    gpiohs_irq_register(10, 1, irq_key2, NULL);

    sysctl_set_spi0_dvp_data(1);
#else
    /* Init DVP IO map and function settings */
    fpioa_set_function(11, FUNC_CMOS_RST);
    fpioa_set_function(13, FUNC_CMOS_PWDN);
    fpioa_set_function(14, FUNC_CMOS_XCLK);
    fpioa_set_function(12, FUNC_CMOS_VSYNC);
    fpioa_set_function(17, FUNC_CMOS_HREF);
    fpioa_set_function(15, FUNC_CMOS_PCLK);
    fpioa_set_function(10, FUNC_SCCB_SCLK);
    fpioa_set_function(9, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(8, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(6, FUNC_SPI0_SS3);
    fpioa_set_function(7, FUNC_SPI0_SCLK);

    sysctl_set_spi0_dvp_data(1);

#endif
}

static void io_set_power(void)
{
#if BOARD_LICHEEDAN
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
#else
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK2, SYSCTL_POWER_V18);
#endif
}




static int reverse_u32pixel(uint32_t* addr,uint32_t length)
{
  if(NULL == addr)
    return -1;

  uint32_t data;
  uint32_t* pend = addr+length;
  for(;addr<pend;addr++)
  {
	  data = *(addr);
	  *(addr) = ((data & 0x000000FF) << 24) | ((data & 0x0000FF00) << 8) | 
                ((data & 0x00FF0000) >> 8) | ((data & 0xFF000000) >> 24) ;
  }  //1.7ms
  return 0;
}

uint8_t buffer[1024  * 1024];
static void sendJpeg(void)
{
    image_t img;    
    img.w = 320;
    img.h = 240;
    img.bpp = 2;    

    uint32_t* tmp_buffer = (uint8_t *)(g_ram_mux ? g_lcd_gram0 : g_lcd_gram1);
    reverse_u32pixel((tmp_buffer), img.w * img.h /2);

    img.pixels = (uint8_t*)tmp_buffer;    
    image_t out = { .w=img.w, .h=img.h, .bpp=sizeof(buffer), .pixels=buffer };

    
    jpeg_compress(&img, &out, 80, false);    
    //文件大小 out.bpp
    //文件内容 out.pixels
    for(int i = 0; i < out.bpp; i++)
    {
      uarths_putchar(out.pixels[i]);
    }
}

int main(void)
{
    uint64_t core_id = current_coreid();

    if (core_id == 0)
    {
        /* Set CPU and dvp clk */
        sysctl_pll_set_freq(SYSCTL_PLL0, 800000000UL);
        sysctl_pll_set_freq(SYSCTL_PLL1, 300000000UL);
        sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
        uarths_init();
        plic_init();
        io_mux_init();
        io_set_power();
        

        /* LCD init */
//        printf("LCD init\n");
        lcd_init();
#if BOARD_LICHEEDAN
#if OV5640
        lcd_set_direction(DIR_YX_RLUD);
#else
        lcd_set_direction(DIR_YX_RLDU);
#endif
#else
#if OV5640
        lcd_set_direction(DIR_YX_LRUD);
#else
        lcd_set_direction(DIR_YX_LRDU);
#endif
#endif

        lcd_clear(BLACK);

        /* DVP init */
//        printf("DVP init\n");
#if OV5640
        dvp_init(16);
        dvp_enable_burst();
        dvp_set_output_enable(0, 1);
        dvp_set_output_enable(1, 1);
        dvp_set_image_format(DVP_CFG_RGB_FORMAT);
        dvp_set_image_size(320, 240);
        ov5640_init();
#else
        dvp_init(8);
        dvp_set_xclk_rate(24000000);
        dvp_enable_burst();
        dvp_set_output_enable(0, 1);
        dvp_set_output_enable(1, 1);
        dvp_set_image_format(DVP_CFG_RGB_FORMAT);
        dvp_set_image_size(320, 240);
        ov2640_init();
#endif

        dvp_set_ai_addr((uint32_t)0x40600000, (uint32_t)0x40612C00, (uint32_t)0x40625800);
        dvp_set_display_addr((uint32_t)g_lcd_gram0);
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
        dvp_disable_auto();

        /* DVP interrupt config */
        //printf("DVP interrupt config\r\n");
        plic_set_priority(IRQN_DVP_INTERRUPT, 1);
        plic_irq_register(IRQN_DVP_INTERRUPT, on_irq_dvp, NULL);
        plic_irq_enable(IRQN_DVP_INTERRUPT);

        /* enable global interrupt */
        sysctl_enable_irq();

        /* system start */
        //printf("system start\r\n");
        g_ram_mux = 0;
        g_dvp_finish_flag = 0;
        dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);

        while (1)
        {
            /* ai cal finish*/
            while (g_dvp_finish_flag == 0)
                ;
            g_dvp_finish_flag = 0;
            /* display pic*/
            g_ram_mux ^= 0x01;

            lcd_draw_picture(0, 0, 320, 240, g_ram_mux ? g_lcd_gram0 : g_lcd_gram1);


            //按键检测
            if(g_gpio_flag)
            {
                uint64_t v_time_now = sysctl_get_time_us();
                if(v_time_now - g_gpio_time > 10*1000)
                {
                    if(gpiohs_get_pin(8) == 1)/*press */
                    {
                        sendJpeg();
                        g_key_press = 1;
                        g_gpio_flag = 0;
                    }
                }
                if(v_time_now - g_gpio_time > 1 * 1000 * 1000) /* 长按1s */
                {
                    if(gpiohs_get_pin(8) == 0)
                    {
                        /* Del All */
                        //printf("Del All feature!\n");                        
                        g_gpio_flag = 0;
                    }
                }
            }
        }
    }
    while (1)
        ;

    return 0;
}
