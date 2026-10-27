//
// emu.h - Capa de compatibilidad mínima para usar código de MAME en ZXMetalCircle
//
// Implementa solo lo que necesitan upd765.cpp y fdc_pll.cpp: attotime, temporizadores
// (emu_timer) sincronizados con el tiempo emulado, device_t, callbacks devcb y macros.
// No hay estados guardados ni depurador: esas partes son vacías.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
#ifndef ZX_MAME_EMU_H
#define ZX_MAME_EMU_H

#include <stdint.h>
#include <string.h>

#ifdef __circle__
#include <circle/types.h>		// mismos nombres de tipos que Circle
#else
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
#endif

#ifndef assert
#define assert(x)	((void) 0)
#endif

#undef BIT				// Circle define BIT(n) con otro significado
#define BIT(x, n)	(((x) >> (n)) & 1)
#define ASSERT_LINE	1
#define CLEAR_LINE	0
#define ATTR_COLD
#define FUNC(x)		&x
#define NAME(x)		x
#define STRUCT_MEMBER(s, m)	s

//------------------------------------------------------------------------------
// attotime: tiempo emulado. Se guarda en picosegundos (64 bits: ±106 días),
// suficiente para una celda MFM (2 us) y para un t-state (~285 ns).

class attotime
{
public:
	static constexpr s64 NEVER = INT64_MAX;

	constexpr attotime () : m_ps (0) {}
	constexpr explicit attotime (s64 ps) : m_ps (ps) {}

	static constexpr attotime from_ps (s64 ps)		{ return attotime (ps); }
	static constexpr attotime from_nsec (s64 ns)		{ return attotime (ns * 1000); }
	static constexpr attotime from_usec (s64 us)		{ return attotime (us * 1000000); }
	static constexpr attotime from_msec (s64 ms)		{ return attotime (ms * 1000000000LL); }
	static constexpr attotime from_seconds (s64 s)		{ return attotime (s * 1000000000000LL); }
	static attotime from_hz (u32 hz)			{ return attotime (1000000000000LL / hz); }
	static attotime from_hz (int hz)			{ return attotime (1000000000000LL / hz); }
	static attotime from_hz (double hz)			{ return attotime ((s64) (1e12 / hz)); }
	static attotime from_double (double s)			{ return attotime ((s64) (s * 1e12)); }

	double as_double () const	{ return is_never () ? 1e30 : m_ps * 1e-12; }
	s64 as_ps () const		{ return m_ps; }
	s64 seconds () const		{ return m_ps < 0 ? -1 - (-m_ps - 1) / 1000000000000LL : m_ps / 1000000000000LL; }
	bool is_never () const		{ return m_ps == NEVER; }
	bool is_zero () const		{ return m_ps == 0; }
	const char *to_string () const	{ return ""; }	// solo se usa en mensajes desactivados

	static const attotime never;
	static const attotime zero;

	attotime operator+ (const attotime &o) const
		{ return (is_never () || o.is_never ()) ? attotime (NEVER) : attotime (m_ps + o.m_ps); }
	attotime operator- (const attotime &o) const
		{ return is_never () ? attotime (NEVER) : attotime (m_ps - o.m_ps); }
	attotime operator* (u32 n) const	{ return is_never () ? *this : attotime (m_ps * n); }
	attotime operator/ (u32 n) const	{ return is_never () ? *this : attotime (m_ps / n); }
	attotime &operator+= (const attotime &o)	{ *this = *this + o; return *this; }
	attotime &operator-= (const attotime &o)	{ *this = *this - o; return *this; }

	bool operator< (const attotime &o) const	{ return m_ps < o.m_ps; }
	bool operator<= (const attotime &o) const	{ return m_ps <= o.m_ps; }
	bool operator> (const attotime &o) const	{ return m_ps > o.m_ps; }
	bool operator>= (const attotime &o) const	{ return m_ps >= o.m_ps; }
	bool operator== (const attotime &o) const	{ return m_ps == o.m_ps; }
	bool operator!= (const attotime &o) const	{ return m_ps != o.m_ps; }

private:
	s64 m_ps;
};

//------------------------------------------------------------------------------
// Temporizadores. El tiempo avanza solo cuando el emulador llama a
// mame_scheduler::advance_to(); entonces vencen, en orden, los que toquen.

class emu_timer;

class mame_scheduler
{
public:
	static attotime now ()			{ return s_now; }
	static void advance_to (const attotime &t);	// ejecuta los temporizadores vencidos
	static void add (emu_timer *timer);
	static void remove (emu_timer *timer);
	static void changed ()			{ s_bDirty = true; }

private:
	static attotime s_now;
	static attotime s_next;		// vencimiento más próximo (si !s_bDirty)
	static bool s_bDirty;
	static emu_timer *s_list;
	friend class emu_timer;
};

class emu_timer
{
public:
	typedef void (*thunk_t) (void *obj, s32 param);

	emu_timer (void *obj, thunk_t thunk) : m_obj (obj), m_thunk (thunk), m_expire (attotime::never),
		m_period (attotime::never), m_param (0), m_enabled (false), m_next (0)
		{ mame_scheduler::add (this); }

	void adjust (attotime start, s32 param = 0, attotime period = attotime::never)
	{
		m_param = param;
		m_period = period;
		m_expire = start.is_never () ? attotime::never : mame_scheduler::now () + start;
		m_enabled = !start.is_never ();
		mame_scheduler::changed ();
	}
	void enable (bool on = true)	{ m_enabled = on; mame_scheduler::changed (); }
	bool enabled () const		{ return m_enabled; }
	attotime remaining () const	{ return m_enabled ? m_expire - mame_scheduler::now () : attotime::never; }
	attotime expire () const	{ return m_enabled ? m_expire : attotime::never; }
	void reset (attotime t = attotime::never)	{ adjust (t, m_param, m_period); }

private:
	void *m_obj;
	thunk_t m_thunk;
	attotime m_expire, m_period;
	s32 m_param;
	bool m_enabled;
	emu_timer *m_next;
	friend class mame_scheduler;
};

//------------------------------------------------------------------------------
// Máquina y dispositivos

class running_machine
{
public:
	attotime time () const			{ return mame_scheduler::now (); }
	bool side_effects_disabled () const	{ return false; }
	const char *describe_context () const	{ return ""; }
};

class machine_config {};
typedef const char *device_type;

#define DECLARE_DEVICE_TYPE(Type, Class)		extern const device_type Type;
#define DEFINE_DEVICE_TYPE(Type, Class, Short, Name)	const device_type Type = Short;

class address_map {};

class device_t
{
public:
	device_t (const machine_config &, device_type type, const char *tag, device_t *owner, u32 clock)
		: m_type (type), m_tag (tag), m_clock (clock) {}
	virtual ~device_t () {}

	// arranque (lo que en MAME hace el sistema de dispositivos)
	void start ()			{ device_start (); device_reset (); }
	void reset ()			{ device_reset (); }

	running_machine &machine () const	{ static running_machine m; return m; }
	u32 clock () const		{ return m_clock; }
	const char *tag () const	{ return m_tag; }

	template <class C>
	emu_timer *timer_alloc (void (C::*func) (s32), C *obj)
	{
		struct Binder
		{
			void (C::*f) (s32);
			C *o;
			static void call (void *b, s32 p) { Binder *x = static_cast<Binder *> (b); (x->o->*x->f) (p); }
		};
		Binder *b = new Binder;
		b->f = func;
		b->o = obj;
		return new emu_timer (b, &Binder::call);
	}

	template <typename T> void save_item (const T &) {}
	template <class T> T *subdevice (const char *) { return nullptr; }
	void logerror (const char *, ...) const {}

protected:
	virtual void device_start () {}
	virtual void device_reset () {}

private:
	device_type m_type;
	const char *m_tag;
	u32 m_clock;
};

#define TIMER_CALLBACK_MEMBER(name)	void name (s32 param)

//------------------------------------------------------------------------------
// Callbacks (devcb). Solo una función C por callback.

class devcb_write_line
{
public:
	typedef void (*func_t) (void *ctx, int state);
	explicit devcb_write_line (device_t &, int = 0) : m_func (0), m_ctx (0) {}
	devcb_write_line &bind () { return *this; }
	void set (func_t f, void *ctx)	{ m_func = f; m_ctx = ctx; }
	void operator() (int state)	{ if (m_func) m_func (m_ctx, state); }
	bool isunset () const		{ return m_func == 0; }
private:
	func_t m_func;
	void *m_ctx;
};

class devcb_write8
{
public:
	typedef void (*func_t) (void *ctx, u8 data);
	explicit devcb_write8 (device_t &) : m_func (0), m_ctx (0) {}
	devcb_write8 &bind () { return *this; }
	void set (func_t f, void *ctx)	{ m_func = f; m_ctx = ctx; }
	void operator() (u8 data)	{ if (m_func) m_func (m_ctx, data); }
	void operator() (int, u8 data)	{ (*this) (data); }
	bool isunset () const		{ return m_func == 0; }
private:
	func_t m_func;
	void *m_ctx;
};

class devcb_read_line
{
public:
	typedef int (*func_t) (void *ctx);
	explicit devcb_read_line (device_t &, int dflt = 0) : m_func (0), m_ctx (0), m_default (dflt) {}
	devcb_read_line &bind () { return *this; }
	void set (func_t f, void *ctx)	{ m_func = f; m_ctx = ctx; }
	int operator() ()		{ return m_func ? m_func (m_ctx) : m_default; }
	bool isunset () const		{ return m_func == 0; }
private:
	func_t m_func;
	void *m_ctx;
	int m_default;
};

// La disquetera se conecta con set_floppy(): no hay conectores
class floppy_image_device;
class floppy_connector
{
public:
	floppy_image_device *get_device ()	{ return 0; }
};

namespace util
{
	struct fake_string { const char *c_str () const { return ""; } };
	template <typename... Args> fake_string string_format (const char *, Args...) { return fake_string (); }
}

#endif
