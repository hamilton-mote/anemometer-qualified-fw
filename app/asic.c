
#include "asic.h"
#include <periph/gpio.h>
#include <string.h>
#include "gpb_v1.0_wind.rawbin.h"
#include "wind_v8.rawbin.h"
#include "wind_v10.rawbin.h"
#define PROG_ADDR 0x85
#define PROG_CNT  0x87
#define PROG_CTL  0xC4
#define PROG_DATA 0xC6
#define PROG_CPU  0xC2
#define DEF_ADDR  0x45
//#define DEF_ADDR  0x8a

#define ASIC_FIRMWARE_ARRAY wind_v10_rawbin
#define OPMODE 0x01
#define OPMODE_SZ 01
#define TICK_INTERVAL 0x02
#define TICK_INTERVAL_SZ 2
#define PERIOD 0x05
#define PERIOD_SZ 1
#define CAL_TRIG 0x06
#define CAL_TRIG_SZ 1
#define MAX_RANGE 0x07
#define MAX_RANGE_SZ 1
#define CAL_RESULT 0x0A
#define CAL_RESULT_SZ 2
#define HOLDOFF    0x11
#define HOLDOFF_SZ 1
#define ST_RANGE 0x12
#define ST_RANGE_SZ 1
#define ST_COEFF 0x13
#define ST_COEFF_SZ 1
#define READY 0x14
#define READY_SZ 1
#define TOF_SF 0x16
#define TOF_SF_SZ 2
#define TOF 0x18
#define TOF_SZ 2
#define INTENSITY 0x1A
#define INTENSITY_SZ 2

#define MODE_TXRX 0x10
#define MODE_RX 0x20

#define PEX_ADDR_A 0x22
#define PEX_ADDR_B 0x21

#define E_OK 0
#define E_NOTIMPL -1
#define E_FAIL -2
#define E_NOTREADY -3
#define E_CALFAIL -4

#define PXA_RST_0 0x0001
#define PXA_RST_1 0x0002
#define PXA_RST_2 0x0004
#define PXA_RST_3 0x0008
#define PXA_RST_4 0x0010
#define PXA_RST_5 0x0020
#define PXA_LED_0 0x0040
#define PXA_LED_1 0x0080

#define PXA_PROG_0 0x0100
#define PXA_PROG_1 0x0200
#define PXA_PROG_2 0x0400
#define PXA_PROG_3 0x0800
#define PXA_PROG_4 0x1000
#define PXA_PROG_5 0x2000
#define PXA_LED_2  0x4000

#define PXB_INT0_EN 0x0001
#define PXB_INT1_EN 0x0002
#define PXB_INT2_EN 0x0004
#define PXB_INT3_EN 0x0008
#define PXB_INT4_EN 0x0010
#define PXB_INT5_EN 0x0020

#define GINT (GPIO_PIN(0,18))

#ifdef ROOM_TYPE
#define DEF_MAXRANGE 0x10
//#define READ_OFFSET 0
#elif defined(DUCT_TYPE) || defined(DUCT6_TYPE)
//#define READ_OFFSET 12
#define DEF_MAXRANGE 50
#endif

#ifndef READ_OFFSET
#error READ_OFFSET undefined
#endif

#define SAMPLE_SIZE 70
#define CAL_US 160000
#define SAMPLE_US 10000

int8_t write_pex_a(asic_tetra_t *a)
{
  uint8_t buf [3];
  buf[0] = 0x02;
  buf[1] = a->shadowLA;
  buf[2] = a->shadowHA;
  if (i2c_write_bytes(a->pex_i2c, PEX_ADDR_A, (char*)buf, 3) != 3)
    return E_FAIL;
  return E_OK;
}
int8_t write_pex_b(asic_tetra_t *a)
{
  uint8_t buf [3];
  buf[0] = 0x02;
  buf[1] = a->shadowLB;
  buf[2] = a->shadowHB;
  if (i2c_write_bytes(a->pex_i2c, PEX_ADDR_B, (char*)buf, 3) != 3)
    return E_FAIL;
  return E_OK;
}

void attempt_clockout_hack(i2c_t i2c) {
  gpio_init(GPIO_PIN(0,17), GPIO_OUT);
  gpio_init(GPIO_PIN(0,16), GPIO_IN);

  for (int i = 0; i < 32; i ++) {
      gpio_write(GPIO_PIN(0,17), 0);
    xtimer_usleep(100);
      gpio_write(GPIO_PIN(0,17), 1);
    xtimer_usleep(100);
  }
  i2c_init_master(i2c, I2C_SPEED_NORMAL);
}
int8_t asic_init(asic_tetra_t *a, i2c_t pex_i2c, i2c_t asic_i2c)
{
  a->pex_i2c = pex_i2c;
  a->asic_i2c = asic_i2c;
  a->addr[0] = 0x30;
  a->addr[1] = 0x32;
  a->addr[2] = 0x33;
  a->addr[3] = 0x38;
  a->addr[4] = 0x35;
  a->addr[5] = 0x36;
  a->shadowHA = 0;
  a->shadowLA = 0;
  a->shadowHB = 0;
  a->shadowLB = 0xFF;

  // acquire exclusive access to the bus
  if (i2c_acquire(pex_i2c))
    return E_FAIL;

  // initialize the I2C bus
  if (i2c_init_master(pex_i2c, I2C_SPEED_FAST))
    return E_FAIL;

  // initialize the port expander
  uint8_t buf [3];
  buf[0] = 0x06; //write to cfg port A.0
  buf[1] = 0; //All output
  if (i2c_write_bytes(pex_i2c, PEX_ADDR_A, (char*)buf, 2) != 2) {
    return E_FAIL;
  }


  buf[0] = 0x07; //write to cfg port A.1
  buf[1] = 0; //All output
  if (i2c_write_bytes(pex_i2c, PEX_ADDR_A, (char*)buf, 2) != 2)
    return E_FAIL;

  buf[0] = 0x06; //write to cfg port B.0
  buf[1] = 0; //All output
  if (i2c_write_bytes(pex_i2c, PEX_ADDR_B, (char*)buf, 2) != 2)
    return E_FAIL;

  buf[0] = 0x07; //write to cfg port B.1
  buf[1] = 0xFF; //All input
  if (i2c_write_bytes(pex_i2c, PEX_ADDR_B, (char*)buf, 2) != 2)
    return E_FAIL;

  write_pex_b(a);

  if (i2c_release(pex_i2c))
    return E_FAIL;

  // acquire exclusive access to the bus
  if (i2c_acquire(asic_i2c))
    return E_FAIL;

  // initialize the I2C bus
  if (i2c_init_master(asic_i2c, I2C_SPEED_FAST))
    return E_FAIL;

  if (i2c_release(asic_i2c))
    return E_FAIL;

  if (gpio_init(GINT, GPIO_OUT))
    return E_FAIL;

  // while(1) {
  //
  //   for (int i = 0; i < 10000; i++) {
  //     gpio_write(GINT, 1);
  //   }
  //   for (int i = 0; i < 10000; i++) {
  //     gpio_write(GINT, 0);
  //   }
  // }
  return E_OK;
}

void pex_set_a(asic_tetra_t *a, uint16_t bits)
{
  a->shadowLA |= bits & 0xFF;
  a->shadowHA |= (bits >> 8);
}
void pex_clr_a(asic_tetra_t *a, uint16_t bits)
{
  a->shadowLA &= ~(bits &0xFF);
  a->shadowHA &= ~(bits >> 8);
}
void pex_set_b(asic_tetra_t *a, uint16_t bits)
{
  a->shadowLB |= bits & 0xFF;
  a->shadowHB |= (bits >> 8);
}
void pex_clr_b(asic_tetra_t *a, uint16_t bits)
{
  a->shadowLB &= ~(bits &0xFF);
  a->shadowHB &= ~(bits >> 8);
}
int8_t asic_led(asic_tetra_t *a, uint8_t red, uint8_t green, uint8_t orange)
{
  if (i2c_acquire(a->pex_i2c))
    return E_FAIL;
  if (red) {
    pex_set_a(a, PXA_LED_1);
  } else {
    pex_clr_a(a, PXA_LED_1);
  }
  if (green) {
    pex_set_a(a, PXA_LED_2);
  } else {
    pex_clr_a(a, PXA_LED_2);
  }
  if (orange) {
    pex_set_a(a, PXA_LED_0);
  } else {
    pex_clr_a(a, PXA_LED_0);
  }
  int8_t e = write_pex_a(a);
  int8_t e2 = i2c_release(a->pex_i2c);
  if (e) return e;
  if (e2) return e2;
  return E_OK;
}
int8_t cfg_rst(asic_tetra_t *a, uint8_t num, uint8_t val)
{
  if (val) {
    pex_set_a(a, PXA_RST_0 << num);
  } else {
    pex_clr_a(a, PXA_RST_0 << num);
  }
  return write_pex_a(a);
}
int8_t cfg_prog(asic_tetra_t *a, uint8_t num, uint8_t val)
{
  if (val) {
    pex_set_a(a, PXA_PROG_0 << num);
  } else {
    pex_clr_a(a, PXA_PROG_0 << num);
  }
  return write_pex_a(a);
}
int8_t cfg_int_en(asic_tetra_t *a, uint8_t num, uint8_t val)
{
  if (val) {
    pex_set_b(a, PXB_INT0_EN << num);
  } else {
    pex_clr_b(a, PXB_INT0_EN << num);
  }
  return write_pex_b(a);
}
void set_gint(asic_tetra_t *a, uint8_t val)
{
  gpio_write(GINT, val);
}
//The i2c lock must be held when calling this function
int8_t _write_reg(asic_tetra_t *a, uint8_t num, uint8_t addr, uint8_t len, uint8_t *src)
{
  uint8_t buf[8];
  if (len > 6) return E_NOTIMPL;
  buf[0] = addr;
  buf[1] = len;
  for (int i = 0; i < len; i++) buf[2+i] = src[i];
  return (i2c_write_bytes(a->asic_i2c, a->addr[num], (char*)buf, len+2) == len+2) ? E_OK : E_FAIL;
}
int8_t _read_reg(asic_tetra_t *a, uint8_t num, uint8_t addr, uint8_t len, uint8_t *dst)
{
  int rv = i2c_read_regs(a->asic_i2c, a->addr[num], addr, (char*)dst, len);
  if(rv != len) {
    printf("got rv %d\n", rv);
    return E_FAIL;
  }
  return E_OK;
}
//The i2c lock must be held
int8_t check_ready(asic_tetra_t *a, uint8_t num)
{
  int8_t e;
  uint8_t rv;
  e = _read_reg(a, num, READY, 1, &rv);
  if (e) return e;
  if (rv != 0x02) return E_NOTREADY;
  return E_OK;
}
int8_t asic_check_ready(asic_tetra_t *a, uint8_t num)
{
  if (i2c_acquire(a->asic_i2c)) return E_FAIL;
  int8_t e = check_ready(a, num);
  if (i2c_release(a->asic_i2c)) return E_FAIL;
  return e;
}
//The i2c lock must be held
int8_t set_opmode(asic_tetra_t *a, uint8_t num, uint8_t mode)
{
  return _write_reg(a, num, OPMODE, 1, &mode);
}
int8_t prime_calibrate(asic_tetra_t *a, uint8_t num)
{
  uint8_t one = 1;
  return _write_reg(a, num, CAL_TRIG, 1, &one);
}
int32_t read_cal_result(asic_tetra_t *a, uint8_t num)
{
  uint8_t buf[2];
  int8_t e = _read_reg(a, num, CAL_RESULT, 2, buf);
  if (e) {
    printf("rcalres %d failed %d\n", num, e);
    return E_FAIL;
  }
  return (int32_t)(buf[0]) + ((int32_t)(buf[1]) << 8);

}
int8_t asic_calibrate(asic_tetra_t *a)
{
  int8_t e;
  set_gint(a, 0);
  if (i2c_acquire(a->asic_i2c)) return -2;
  for (int i = 0; i < NUMASICS; i++)
  {
    e = set_opmode(a, i, 0x01);
    if (e) goto end;
  }
  xtimer_usleep(30);
  for (int8_t i = 0; i < NUMASICS; i++)
  {
    e = prime_calibrate(a, i);
    if (e) goto end;
  }
  set_gint(a, 1);
  xtimer_usleep(CAL_US);
  set_gint(a, 0);
  a->cal_pulse = CAL_US/1000;
  for (int8_t i = 0; i < NUMASICS; i++)
  {
    int32_t r = read_cal_result(a, i);
    if (r < 0) {
      e = E_CALFAIL;
      goto end;
    }
    a->calres[i] = (uint16_t) r;
  //  xtimer_usleep(200);
  }
  end:
  if (i2c_release(a->asic_i2c)) return E_FAIL;
  return e;
}

int8_t set_max_range(asic_tetra_t *a, uint8_t num, uint8_t val)
{
  return _write_reg(a, num, MAX_RANGE, 1, &val);
}
// int8_t read_sample_data(asic_tetra_t *a, uint8_t num, uint8_t *dst)
// {
//   #ifdef ROOM_TYPE
//   return _read_reg(a, num, TOF_SF, SAMPLE_SIZE, dst);
//   #elif defined(DUCT_TYPE)
//   int rv1 = _read_reg(a, num, TOF_SF, 6, &dst[0]);
//   if (rv1) {
//     return rv1;
//   }
//   int rv2 = _read_reg(a, num, 0x4C, 64, &dst[6]);
//   return rv2;
//   #else
//   #error define type yo, what do you think this is?
//   #endif
// }
//For a room anemometer, read from 0..16
//For a duct anemometer, read from 12..28
int8_t read_16_iq_points(asic_tetra_t *a, uint8_t num, uint8_t *dst, uint8_t from)
{
  return _read_reg(a, num, 0x1c + (from<<2), 64, &dst[0]);
  #if 0
    #ifdef ROOM_TYPE
      return _read_reg(a, num, 0x1c, 64, &dst[0]);
    #elif defined(DUCT_TYPE) || defined(DUCT6_TYPE)
      return _read_reg(a, num, 0x4c, 64, &dst[0]);
    #else
      #error define type yo, what do you think this is?
    #endif
  #endif
}
int8_t asic_program(asic_tetra_t *a, uint8_t num)
{
  if (i2c_acquire(a->asic_i2c)) return -10;
  if (cfg_rst(a, num, 0)) return -11;
  if (cfg_prog(a, num, 1)) return -12;
  if (cfg_rst(a, num, 1)) return -13;
  xtimer_usleep(50000);
  uint8_t buf[3];
  buf[0] = PROG_ADDR;
  buf[1] = 0x00;
  buf[2] = 0xF8;
  if (i2c_write_bytes(a->asic_i2c, DEF_ADDR, (char*) buf, 3) != 3) {
    i2c_release(a->asic_i2c);
    return -14;
  }
  buf[0] = PROG_CNT;
  buf[1] = 0xFF;
  buf[2] = 0x07;
  if (i2c_write_bytes(a->asic_i2c, DEF_ADDR, (char*) buf, 3) != 3) {
    i2c_release(a->asic_i2c);
    return -15;
  }
  if (i2c_write_bytes(a->asic_i2c, DEF_ADDR, (char*)ASIC_FIRMWARE_ARRAY, sizeof(ASIC_FIRMWARE_ARRAY)) != sizeof(ASIC_FIRMWARE_ARRAY)) {
    i2c_release(a->asic_i2c);
    return -16;
  }
  buf[0] = PROG_ADDR;
  buf[1] = 0xC5;
  buf[2] = 0x01;
  if (i2c_write_bytes(a->asic_i2c,DEF_ADDR, (char*) buf, 3) != 3) {
    i2c_release(a->asic_i2c);
    return -17;
  }
  buf[0] = PROG_DATA;
  buf[1] = a->addr[num];
  if (i2c_write_bytes(a->asic_i2c, DEF_ADDR, (char*) buf, 2) != 2) {
    i2c_release(a->asic_i2c);
    return -18;
  }
  buf[0] = PROG_CTL;
  buf[1] = 0x0B;
  if (i2c_write_bytes(a->asic_i2c, DEF_ADDR, (char*) buf, 2) != 2) {
    i2c_release(a->asic_i2c);
    return -19;
  }
  buf[0] = PROG_CPU;
  buf[1] = 0x02;
  if (i2c_write_bytes(a->asic_i2c, DEF_ADDR, (char*) buf, 2) != 2) {
    i2c_release(a->asic_i2c);
    return -20;
  }
  if (cfg_prog(a, num, 0)) return -22;
  //if (cfg_rst(a, num, 0)) return -23;
  if (i2c_release(a->asic_i2c)) return -21;
  //leave it in reset
  return E_OK;
}
int8_t asic_all_out_of_reset(asic_tetra_t *a)
{
  if (i2c_acquire(a->asic_i2c)) return -2;
  for (int8_t i = 0; i < NUMASICS; i++)
  {
    if (cfg_rst(a, i, 1)) return -3;
  }
  if (i2c_release(a->asic_i2c)) return -4;
  return E_OK;
}
int8_t asic_configure(asic_tetra_t *a, uint8_t num)
{
  if (i2c_acquire(a->asic_i2c)) return -2;
  if (cfg_rst(a, num, 1)) return -3;
  if (set_max_range(a, num, DEF_MAXRANGE)) return -4;
  if (cfg_int_en(a, num, 1)) return -5;

  //moar?
  if (i2c_release(a->asic_i2c)) return -5;
  return E_OK;
}

int8_t asic_fake_measure(asic_tetra_t *a)
{
  int8_t e;
  if (i2c_acquire(a->asic_i2c)) return E_FAIL;
  for (int i = 0; i < NUMASICS; i++)
  {
    e = set_opmode(a, i, MODE_RX);
    if (e) goto fail;
  }
  xtimer_usleep(300);
  set_gint(a, 1);
  xtimer_usleep(20);
  set_gint(a, 0);
  xtimer_usleep(SAMPLE_US);
  e = E_OK;

  for (int i = 0; i < NUMASICS; i++)
  {
    e = set_opmode(a, i, 0x01);
    if (e) goto fail;
  }

  fail:
  i2c_release(a->asic_i2c);
  return e;
}
// int8_t asic_measure(asic_tetra_t *a, uint8_t primary, measurement_t *m)
// {
//   int8_t e;
//   if (i2c_acquire(a->i2c)) return E_FAIL;
//   for (int i = 0; i < 4; i++)
//   {
//     if (i == primary)
//     {
//       e = set_opmode(a, i, MODE_TXRX);
//     }
//     else
//     {
//       e = set_opmode(a, i, MODE_RX);
//     }
//     if (e) goto fail;
//   }
//   set_gint(a, 1);
//   xtimer_usleep(20);
//   set_gint(a, 0);
//   xtimer_usleep(SAMPLE_US);
//   for (int i = 0; i < 4; i++)
//   {
//     e = read_sample_data(a, i, &(m->sampledata[i][0]));
//     if (e) {
//       e = - (40+i);
//       goto fail;
//     }
//   }
//   m->primary = primary;
//   e = E_OK;
//   fail:
//   i2c_release(a->i2c);
//   return e;
// }
int8_t asic_measure_just_iq(asic_tetra_t *a, uint8_t primary, measurement_t *m)
{
  int8_t e;
  int slotindex = 0;
  if (i2c_acquire(a->asic_i2c)) return E_FAIL;
  for (int i = 0; i < NUMASICS; i++)
  {
    if (i == primary)
    {
      e = set_opmode(a, i, MODE_TXRX);
    }
    else
    {
      e = set_opmode(a, i, MODE_RX);
    }

    if (e) goto fail;
  }
  xtimer_usleep(300);
  set_gint(a, 1);
  xtimer_usleep(20);
  set_gint(a, 0);
  xtimer_usleep(40);
  //Workaround, set OPMODE to stop to prevent retrigger
  for (int i = 0; i < NUMASICS; i++)
  {
    e = set_opmode(a, i, 0x01);
    if (e) goto fail;
  }

  xtimer_usleep(SAMPLE_US);

  for (int i = 0; i < NUMASICS; i++)
  {
    #if defined(DUCT6_TYPE)
    if (primary < 3 && i < 3) {
      continue;
    }
    if (primary >= 3 && i >= 3) {
      continue;
    }
    #else
    if (i == primary) {
      continue;
    }
    #endif

    uint8_t holdbuf [4];
    e = _read_reg(a, i, TOF_SF, 4, &holdbuf[0]);
    if (e) {
      e = - (40+i);
      goto fail;
    }
    m->tof_sf[slotindex] = (uint16_t)(holdbuf[0]) + (((uint16_t)holdbuf[1])<<8);
    #ifdef AUTORANGE
    uint8_t startindex = holdbuf[3];
    if (startindex == 255) {
      m->offset[slotindex] = 255;
      memset(&(m->sampledata[slotindex][0]), 0, 16);
    } else {
      if (startindex >= 4) {
        startindex -= 4;
      }
      m->offset[slotindex] = startindex;
      e = read_16_iq_points(a, i, &(m->sampledata[slotindex][0]), startindex);
      if (e) {
        e = - (40+i);
        goto fail;
      }
    }
    #else
    m->offset[slotindex] = READ_OFFSET;
    e = read_16_iq_points(a, i, &(m->sampledata[slotindex][0]), READ_OFFSET);
    if (e) {
      e = - (40+i);
      goto fail;
    }
    #endif
    //printf("primary=%d rx=%d maxi=%d\n", primary, primary < 3 ? slotindex + 3 : slotindex, m->offset[slotindex]);
    slotindex++;
  }
  m->primary = primary;
  e = E_OK;
  fail:
  i2c_release(a->asic_i2c);
  return e;
}
