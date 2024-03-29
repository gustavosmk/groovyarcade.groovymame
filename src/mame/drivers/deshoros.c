/***************************************************************************

Destiny (c) 1983 Data East Corporation

driver by Angelo Salese

A fortune-teller machine with 24 characters LED-array and a printer.
M6809 CPU, 2KB RAM

Rough cpanel sketch:

    [LED-array dispay]          CLEAR ENTER
                                7  8  9  0
                                4  5  6  F
                                1  2  3  M

To control system buttons (SYSTEM, lower nibble), hold one down and then
push the main service button F2.


TODO:
- Emulate the graphics with genuine artwork display;
- Printer emulation;
- Exact sound & irq frequency;

***************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "sound/beep.h"

class destiny_state : public driver_device
{
public:
	destiny_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;

	char m_led_array[21];

	DECLARE_WRITE8_MEMBER(firq_ack_w);
	DECLARE_WRITE8_MEMBER(nmi_ack_w);
	DECLARE_READ8_MEMBER(printer_status_r);
	DECLARE_READ8_MEMBER(display_ready_r);
	DECLARE_WRITE8_MEMBER(display_w);
	DECLARE_WRITE8_MEMBER(out_w);
	DECLARE_WRITE8_MEMBER(bank_select_w);
	DECLARE_WRITE8_MEMBER(sound_on_w);
	DECLARE_WRITE8_MEMBER(sound_off_w);

	DECLARE_INPUT_CHANGED_MEMBER(coin_inserted);

protected:
	// driver_device overrides
	virtual void machine_start();
	virtual void machine_reset();
};


/*Temporary,to show something on screen...*/

static VIDEO_START( destiny )
{
	destiny_state *state = machine.driver_data<destiny_state>();
	UINT8 i;
	for(i=0;i<20;i++)
		state->m_led_array[i] = 0x20;
	state->m_led_array[20] = 0;
}

static SCREEN_UPDATE_IND16( destiny )
{
	destiny_state *state = screen.machine().driver_data<destiny_state>();
	popmessage("%s",state->m_led_array);
	return 0;
}



/***************************************************************************

  I/O

***************************************************************************/

WRITE8_MEMBER(destiny_state::firq_ack_w)
{
	device_set_input_line(m_maincpu, M6809_FIRQ_LINE, CLEAR_LINE);
}

WRITE8_MEMBER(destiny_state::nmi_ack_w)
{
	device_set_input_line(m_maincpu, INPUT_LINE_NMI, CLEAR_LINE);
}

READ8_MEMBER(destiny_state::printer_status_r)
{
	// d2: mark sensor
	// d3: motor stop
	// d4: status
	// d5: /L-SW
	// d6: paper
	// d7: /R-SW
	return 0xff;
}

READ8_MEMBER(destiny_state::display_ready_r)
{
	// d7: /display ready
	// other bits: N/C
	return 0;
}

WRITE8_MEMBER(destiny_state::display_w)
{
	/* this is preliminary, just fills a string and doesn't support control codes etc. */

	// scroll the data
	for (int i = 0; i < 19; i++)
		m_led_array[i] = m_led_array[i+1];

	// update
	m_led_array[19] = data;
}

WRITE8_MEMBER(destiny_state::out_w)
{
	// d0: coin blocker
	coin_lockout_w(machine(), 0, ~data & 1);

	// d1: paper cutter 1
	// d2: paper cutter 2
	// other bits: N/C?
}

WRITE8_MEMBER(destiny_state::bank_select_w)
{
	// d0-d2 and d4: bank (but only up to 4 banks supported)
	membank("bank1")->set_base(memregion("answers")->base() + 0x6000 * (data & 3));
}

INPUT_CHANGED_MEMBER(destiny_state::coin_inserted)
{
	// NMI on Coin SW or Service SW
	if (oldval)
		device_set_input_line(m_maincpu, INPUT_LINE_NMI, ASSERT_LINE);

	// coincounter on coin insert
	if (((int)(FPTR)param) == 0)
		coin_counter_w(machine(), 0, newval);
}

WRITE8_MEMBER(destiny_state::sound_on_w)
{
	beep_set_state(machine().device(BEEPER_TAG),1);
}

WRITE8_MEMBER(destiny_state::sound_off_w)
{
	beep_set_state(machine().device(BEEPER_TAG),0);
}

static ADDRESS_MAP_START( main_map, AS_PROGRAM, 8, destiny_state )
	AM_RANGE(0x0000, 0x5fff) AM_ROMBANK("bank1")
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x9000, 0x9000) AM_READWRITE(printer_status_r, firq_ack_w)
	AM_RANGE(0x9001, 0x9001) AM_READ_PORT("SYSTEM") AM_WRITE(nmi_ack_w)
	AM_RANGE(0x9002, 0x9002) AM_READWRITE(display_ready_r, display_w)
	AM_RANGE(0x9003, 0x9003) AM_READ_PORT("KEY1")
	AM_RANGE(0x9004, 0x9004) AM_READ_PORT("KEY2")
	AM_RANGE(0x9005, 0x9005) AM_READ_PORT("DIPSW") AM_WRITE(out_w)
//  AM_RANGE(0x9006, 0x9006) AM_NOP // printer motor on
//  AM_RANGE(0x9007, 0x9007) AM_NOP // printer data
	AM_RANGE(0x900a, 0x900a) AM_WRITE(sound_on_w)
	AM_RANGE(0x900b, 0x900b) AM_WRITE(sound_off_w)
	AM_RANGE(0x900c, 0x900c) AM_WRITE(bank_select_w)
//  AM_RANGE(0x900d, 0x900d) AM_NOP // printer motor off
//  AM_RANGE(0x900e, 0x900e) AM_NOP // printer motor jam reset
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END



/***************************************************************************

  Inputs

***************************************************************************/

static INPUT_PORTS_START( destiny )
	PORT_START("KEY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key Male") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key Female") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 4") PORT_CODE(KEYCODE_4_PAD)

	PORT_START("KEY2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key Enter") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Key Clear") PORT_CODE(KEYCODE_PLUS_PAD)

	PORT_START("DIPSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )		PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPUNKNOWN_DIPLOC( 0x02, 0x00, "SW1:2" )
	PORT_DIPUNKNOWN_DIPLOC( 0x04, 0x00, "SW1:3" )
	PORT_DIPUNKNOWN_DIPLOC( 0x08, 0x00, "SW1:4" )
	PORT_DIPUNKNOWN_DIPLOC( 0x10, 0x00, "SW1:5" )
	PORT_DIPNAME( 0x20,   0x00, "Force Start" )			PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0,   0x00, "Operation Mode" )		PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(      0x00, "Normal Mode" )
//  PORT_DIPSETTING(      0x40, "Normal Mode" ) // dupe
	PORT_DIPSETTING(      0x80, "Test Mode" )
	PORT_DIPSETTING(      0xc0, "I/O Test" )

	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE4 ) PORT_NAME("Paper Cutter Reset")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE3 ) PORT_NAME("Paper Cutter Set")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 ) PORT_NAME("Paper Cutter Point")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_NAME("Spear") // starts game
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED_MEMBER(DEVICE_SELF, destiny_state, coin_inserted, (void *)0)

	PORT_START("SERVICE")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CHANGED_MEMBER(DEVICE_SELF, destiny_state, coin_inserted, (void *)1)
INPUT_PORTS_END



/***************************************************************************

  Machine Config(s)

***************************************************************************/

void destiny_state::machine_start()
{
	beep_set_frequency(machine().device(BEEPER_TAG),800); // TODO: determine exact frequency thru schematics
	beep_set_state(machine().device(BEEPER_TAG),0);
}

void destiny_state::machine_reset()
{
	bank_select_w(*m_maincpu->memory().space(AS_PROGRAM), 0, 0);
}

static MACHINE_CONFIG_START( destiny, destiny_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6809, XTAL_4MHz/2)
	MCFG_CPU_PROGRAM_MAP(main_map)
	MCFG_CPU_PERIODIC_INT(irq0_line_hold, 60) // timer irq controls update speed, frequency needs to be determined yet (2MHz through three 74LS390)

	/* video hardware (dummy) */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(48*8, 16*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 48*8-1, 0*8, 16*8-1)
	MCFG_SCREEN_UPDATE_STATIC(destiny)
	MCFG_PALETTE_LENGTH(16)

	MCFG_VIDEO_START(destiny)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(BEEPER_TAG, BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.50)
MACHINE_CONFIG_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( destiny )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ag12-4.16c", 0xc000, 0x2000, CRC(03b2c850) SHA1(4e2c49a8d80bc559d0f406caddddb85bc107aac0) )
	ROM_LOAD( "ag13-4.17c", 0xe000, 0x2000, CRC(36959ef6) SHA1(9b3ed44416fcda6a8e89d11ad6e713abd4f63d83) )

	ROM_REGION( 0x18000, "answers", 0 )
	ROM_LOAD( "ag00.1a",   0x00000, 0x2000, CRC(77f5bce0) SHA1(20b5257710c5e848893fec107f0d87a473a4ba24) )
	ROM_LOAD( "ag01.3a",   0x02000, 0x2000, CRC(c08e6a74) SHA1(88679ed8bd2b6b8698258baddf8433c0f60a1b64) )
	ROM_LOAD( "ag02.4a",   0x04000, 0x2000, CRC(687c72b5) SHA1(3f2768c9b6247e96d11b4159f6f5c0dfeb2c5075) )
	ROM_LOAD( "ag03.6a",   0x06000, 0x2000, CRC(535dbe83) SHA1(29336539c57d1fa7d42a0ce01884b29e1707e9ad) )
	ROM_LOAD( "ag04.7a",   0x08000, 0x2000, CRC(e6ae8eb7) SHA1(d0e20e438dcfeac9d844d1fd98701a443ea5e4f7) )
	ROM_LOAD( "ag05.9a",   0x0a000, 0x2000, CRC(c2485e40) SHA1(03f6d7c63a45d430a7965e28aaf07e053ecac7a1) )
	ROM_LOAD( "ag06.10a",  0x0c000, 0x2000, CRC(e6e0bbd1) SHA1(fe693d038b05ae18a3c0cfb25a4649dbb10ab2c7) )
	ROM_LOAD( "ag07.12a",  0x0e000, 0x2000, CRC(a62d879d) SHA1(94d07e774df4c9e4e34ae386714372b53b255530) )
	ROM_LOAD( "ag08.13a",  0x10000, 0x2000, CRC(f5822738) SHA1(afe53e875057317033cdd5f4b7614c96cd11193b) )
	ROM_LOAD( "ag09.15a",  0x12000, 0x2000, CRC(ad3c9f2c) SHA1(f665efb65c072a3d3d2e19844ebe0b352c0251d3) )
	ROM_LOAD( "ag10.16a",  0x14000, 0x2000, CRC(c498754a) SHA1(90e215e8e41d32237d1f4b074d93e20eade92e4e) )
	ROM_LOAD( "ag11.18a",  0x16000, 0x2000, CRC(5f7bf9f9) SHA1(281f89c0bccfcc2bdc1d4d0a5b9cc9a8ab2e7869) )
ROM_END

GAME( 1983, destiny,  0,       destiny,  destiny,  0, ROT0, "Data East Corporation", "Destiny - The Fortuneteller (USA)", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
