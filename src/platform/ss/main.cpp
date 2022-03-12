const void* TRACKS_IMA;
const void* TITLE_SCR;

#include "game.h"

#define SH2_INT_IPRA       (*(volatile unsigned short *)0xFFFFFEE2)
#define SH2_WDT_RTCSR      (*(volatile unsigned char *)0xFFFFFE80)
#define SH2_WDT_RTCNT      (*(volatile unsigned char *)0xFFFFFE81)
#define SH2_WDT_RRSTCSR    (*(volatile unsigned char *)0xFFFFFE83)
#define SH2_WDT_WTCSR_TCNT (*(volatile unsigned short *)0xFFFFFE80)
#define SH2_WDT_WRWOVF_RST (*(volatile unsigned short *)0xFFFFFE82)
#define SH2_WDT_VCR        (*(volatile unsigned short *)0xFFFFFEE4)

// extern "C" {
//     volatile uint32 mars_pwdt_ovf_count = 0;
//     volatile uint32 mars_swdt_ovf_count = 0;
// }

volatile int32 gFrameIndex = 0;
int32 g_timer;
int32 fps;

static iso9660_filelist_t _filelist;

void osSetPalette(const uint16* palette)
{
    scu_dma_transfer(0, (void *)VDP2_CRAM_ADDR(0x0000), palette, 256 * 2);
    scu_dma_transfer_wait(0);
}

int32 osGetSystemTimeMS()
{
    return 1000;
    // return int32((mars_pwdt_ovf_count << 8) | SH2_WDT_RTCNT);
}

bool osSaveSettings()
{
    return false;
}

bool osLoadSettings()
{
    return false;
}

bool osCheckSave()
{
    return false;
}

bool osSaveGame()
{
    return false;
}

bool osLoadGame()
{
    return false;
}

void osJoyVibrate(int32 index, int32 L, int32 R)
{
    // nope
}

void updateInput()
{
    smpc_peripheral_digital_t digital;

    smpc_peripheral_process();
    smpc_peripheral_digital_port(1, &digital);

    keys = 0;

    const uint16_t raw = digital.pressed.raw;

    if ((raw & PERIPHERAL_DIGITAL_UP) != 0)    keys |= IK_UP;
    if ((raw & PERIPHERAL_DIGITAL_RIGHT) != 0) keys |= IK_RIGHT;
    if ((raw & PERIPHERAL_DIGITAL_DOWN) != 0)  keys |= IK_DOWN;
    if ((raw & PERIPHERAL_DIGITAL_LEFT) != 0)  keys |= IK_LEFT;
    if ((raw & PERIPHERAL_DIGITAL_A) != 0)     keys |= IK_A;
    if ((raw & PERIPHERAL_DIGITAL_B) != 0)     keys |= IK_B;
    if ((raw & PERIPHERAL_DIGITAL_C) != 0)     keys |= IK_C;
    if ((raw & PERIPHERAL_DIGITAL_X) != 0)     keys |= IK_X;
    if ((raw & PERIPHERAL_DIGITAL_Y) != 0)     keys |= IK_Y;
    if ((raw & PERIPHERAL_DIGITAL_Z) != 0)     keys |= IK_Z;
    if ((raw & PERIPHERAL_DIGITAL_L) != 0)     keys |= IK_L;
    if ((raw & PERIPHERAL_DIGITAL_R) != 0)     keys |= IK_R;
    if ((raw & PERIPHERAL_DIGITAL_START) != 0) keys |= IK_SELECT;
}

void* osLoadLevel(const char* name)
{
    iso9660_filelist_entry_t* file_entry;
    file_entry = NULL;

    char filename[32];

    sprintf(filename, "%s.PKD", name);

    for (uint32_t i = 0; i < _filelist.entries_pooled_count; i++) {
        file_entry = &_filelist.entries[i];

        if ((strcmp(file_entry->name, filename)) == 0) {
            break;
        }

        file_entry = NULL;
    }

    Level* const level = (Level*)dram_cart_area_get();

    assert(file_entry != NULL);
    int ret __unused;
    ret = cd_block_sectors_read(file_entry->starting_fad, level, file_entry->size);
    assert(ret == 0);

    return level;
}

// uint16 pageIndex = 0;

void pageWait()
{
    vdp2_sync_wait();
}

void pageFlip()
{
    // pageIndex ^= 1;
    vdp2_sync();
}

static void vblankOutHandler(void* work)
{
    smpc_peripheral_intback_issue();

    gFrameIndex++;
}

static void initDRAMCart(void)
{
    const uint32_t id = dram_cart_id_get();

    if ((id != DRAM_CART_ID_1MIB) && (id != DRAM_CART_ID_4MIB)) {
        assert(false && "No cart detected");
    }

    if (id == DRAM_CART_ID_1MIB) {
        assert(false && "32-Mbit cart is not detected");
    }
}

static void initVDP2(void)
{
    vdp2_scrn_bitmap_format_t format;

    format.scroll_screen      = VDP2_SCRN_NBG0;
    format.cc_count           = VDP2_SCRN_CCC_PALETTE_256;
    format.bitmap_size.width  = 512;
    format.bitmap_size.height = 256;
    format.color_palette      = 0x00000000;
    format.bitmap_pattern     = VDP2_VRAM_ADDR(0, 0x00000);
    format.sf_type            = VDP2_SCRN_SF_TYPE_NONE;
    format.sf_code            = VDP2_SCRN_SF_CODE_A;
    format.sf_mode            = 0;

    vdp2_sprite_priority_set(0, 0);
    vdp2_sprite_priority_set(1, 0);
    vdp2_sprite_priority_set(2, 0);
    vdp2_sprite_priority_set(3, 0);
    vdp2_sprite_priority_set(4, 0);
    vdp2_sprite_priority_set(5, 0);
    vdp2_sprite_priority_set(6, 0);
    vdp2_sprite_priority_set(7, 0);

    vdp2_scrn_bitmap_format_set(&format);
    vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
    vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* no_trans = */ true);

    vdp2_vram_cycp_t vram_cycp;
    vram_cycp.pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t2 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[0].t3 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[0].t4 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[0].t5 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[0].t6 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[0].t7 = VDP2_VRAM_CYCP_CPU_RW;

    vram_cycp.pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[1].t2 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[1].t3 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[1].t4 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[1].t5 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[1].t6 = VDP2_VRAM_CYCP_CPU_RW;
    vram_cycp.pt[1].t7 = VDP2_VRAM_CYCP_CPU_RW;

    vram_cycp.pt[2].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vram_cycp.pt[3].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vdp2_vram_cycp_set(&vram_cycp);

    vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE), COLOR_RGB1555(1, 0, 0, 0));

    vdp_sync_vblank_out_set(vblankOutHandler, NULL);

    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_224);
    vdp2_tvmd_display_set();

    vdp2_sync();
    vdp2_sync_wait();

    // SH2_WDT_VCR = (65 << 8) | (SH2_WDT_VCR & 0x00FF); // Set exception vector for WDT
    // SH2_INT_IPRA = (SH2_INT_IPRA & 0xFF0F) | 0x0020; // Set WDT INT to priority 2
    // SH2_WDT_WTCSR_TCNT = 0x5A00;
    // SH2_WDT_WTCSR_TCNT = 0xA53E;
}

static void initCD(void)
{
    // There are 28 files
    iso9660_filelist_entry_t* const filelist_entries =
        iso9660_entries_alloc(64);
    assert(filelist_entries != NULL);

    iso9660_filelist_default_init(&_filelist, filelist_entries, 64);
    iso9660_filelist_root_read(&_filelist);
}

int main(void)
{
    initDRAMCart();
    initVDP2();
    initCD();

    clear();
    pageFlip();
    pageWait();

    // SH2_WDT_VCR = (65<<8) | (SH2_WDT_VCR & 0x00FF); // set exception vector for WDT
    // SH2_INT_IPRA = (SH2_INT_IPRA & 0xFF0F) | 0x0020; // set WDT INT to priority 2

    // SH2_WDT_WTCSR_TCNT = 0x5A00;
    // SH2_WDT_WTCSR_TCNT = 0xA53E;

    gameInit(gLevelInfo[gLevelID].name);

    int32 lastFrame = (gFrameIndex >> 1) - 1;
    int32 fpsCounter = 0;
    int32 fpsFrame = gFrameIndex;
    int vsyncRate = ((vdp2_tvmd_tv_standard_get()) == VDP2_TVMD_TV_STANDARD_NTSC) ? 60 : 50;

    assert(vsyncRate == 60);

    while (true) {
            int32 frame = gFrameIndex;

            if (frame - fpsFrame >= vsyncRate) {
                fpsFrame += vsyncRate;
                fps = fpsCounter;
                fpsCounter = 0;
            }

            if (vsyncRate == 50) {
                frame = frame * 6 / 5;  // PAL fix
            }

            frame >>= 1;

            int32 delta = frame - lastFrame;

            if (!delta)
                continue;

            lastFrame = frame;

            updateInput();

            gameUpdate(delta);

            pageWait();

            gameRender();

            for (uint32_t i = 0; i < FRAME_HEIGHT; i++) {
                const uintptr_t fb_src = (uintptr_t)fb + (i * FRAME_WIDTH);

                scu_dma_transfer(0, (void*)VDP2_VRAM_ADDR(0, i << 9), (void*)fb_src, FRAME_WIDTH);
                scu_dma_transfer_wait(0);
            }

            pageFlip();

            fpsCounter++;
        }

    return 0;
}
