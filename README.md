# JPEG Encoder from OpenMV

* Remove dynamic memory alloc

## Usage

> After display one frame,reverse frame before jpeg compress
``` C
    image_t img;    
    img.w = 320;
    img.h = 240;

    //according to openmv,RGB565 format should be 2
    img.bpp = 2;    

    uint32_t* tmp_buffer = (uint8_t *)(g_ram_mux ? g_lcd_gram0 : g_lcd_gram1);

    //reverse bytes for k210
    reverse_u32pixel((tmp_buffer), img.w * img.h /2);

    img.pixels = (uint8_t*)tmp_buffer;    
    image_t out = { .w=img.w, .h=img.h, .bpp=sizeof(buffer), .pixels=buffer };

    /*
    img:input 
    out:output file
    quatity:three level,60、35 and below
    dynamic memory alloc:false
    */
    jpeg_compress(&img, &out, 80, false);    
    //file size in out.bpp
    //file content in out.pixels
```


``` C
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
    }
    return 0;
}

```