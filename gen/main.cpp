#include "jit.h"

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

struct rectangle {
  int min_x, min_y, max_x, max_y;
  rectangle(int x0, int y0, int x1, int y1) : min_x(x0), min_y(y0), max_x(x1), max_y(y1) {}
};

struct bitmap_t {
  int width, height;
  u16 *data;
  bitmap_t(int w, int h) {
    width = w;
    height = h;
    data = new u16[w*h];
    std::fill(data, data+w*h, 0);
  }

  u16 &pix16(int y, int x) { return data[y*width + x]; }
  template<typename Y, typename X> auto jit_pix16(jit::Function *f, Y y, X x) { return f->ext(data) + f->ext(width) * y + x; }
};

struct flowbitmap {
  bitmap_t b;
  flowbitmap() : b(512, 256) {}

  bitmap_t &bitmap() { return b; }
};


class mixer {
public:
  flowbitmap *m_renderer_input_simple_color[4];
  flowbitmap *m_renderer_input_complex_color[4];
  flowbitmap *m_renderer_input_complex_attr[4];
  flowbitmap *m_renderer_output_color[2];
  flowbitmap *m_renderer_output_attr[2];

  u8 m_shadow_value[4][256];

  u16 m_color_mask[8];
  u8 m_colset[4];
  u8 m_cblk[8];
  u8 m_cblk_on[2];
  u8 m_pri[8+2];
  u8 m_inpri_on[4];
  u8 m_colpri[2];
  u8 m_shd_pri[3];
  u8 m_shd_on, m_shd_pri_sel;
  u8 m_bgc_cblk, m_bgc_set, m_colchg_on;
  u8 m_v_inmix, m_v_inmix_on, m_os_inmix, m_os_inmix_on;
  u8 m_v_inbri, m_os_inbri, m_os_inbri_on;
  u8 m_disp;  

  std::unique_ptr<jit::Context> jit_context;
  jit::Function *renderer;

  mixer();

  void render_default(rectangle &cliprect);
  void prepare_jit(jit::Context &jctxt);
  void render_jit(rectangle &cliprect);
};

mixer::mixer()
{
  for(auto &p : m_renderer_input_simple_color) p = new flowbitmap;
  for(auto &p : m_renderer_input_complex_color) p = new flowbitmap;
  for(auto &p : m_renderer_input_complex_attr) p = new flowbitmap;
  for(auto &p : m_renderer_output_color) p = new flowbitmap;
  for(auto &p : m_renderer_output_attr) p = new flowbitmap;
}


void mixer::render_default(rectangle &cliprect)
{
  u16 prixor = m_bgc_set & 8 ? 0x7ff : 0;
  u8 os_inbri_masked = m_os_inbri & m_os_inbri_on;
  u8 v_inmix_masked  = m_v_inmix  & m_v_inmix_on;
  u8 os_inmix_masked = m_os_inmix & m_os_inmix_on;

  u8 disp = m_disp;

  for(int y = cliprect.min_y; y <= cliprect.max_y; y++) {
    const u16 *acolor  = &m_renderer_input_simple_color[0] ->bitmap().pix16(y, cliprect.min_x);
    const u16 *bcolor  = &m_renderer_input_simple_color[1] ->bitmap().pix16(y, cliprect.min_x);
    const u16 *ccolor  = &m_renderer_input_simple_color[2] ->bitmap().pix16(y, cliprect.min_x);
    const u16 *dcolor  = &m_renderer_input_simple_color[3] ->bitmap().pix16(y, cliprect.min_x);
    const u16 *ocolor  = &m_renderer_input_complex_color[0]->bitmap().pix16(y, cliprect.min_x);
    const u16 *oattr   = &m_renderer_input_complex_attr[0] ->bitmap().pix16(y, cliprect.min_x);
    const u16 *s1color = &m_renderer_input_complex_color[1]->bitmap().pix16(y, cliprect.min_x);
    const u16 *s1attr  = &m_renderer_input_complex_attr[1] ->bitmap().pix16(y, cliprect.min_x);
    const u16 *s2color = &m_renderer_input_complex_color[2]->bitmap().pix16(y, cliprect.min_x);
    const u16 *s2attr  = &m_renderer_input_complex_attr[2] ->bitmap().pix16(y, cliprect.min_x);
    const u16 *s3color = &m_renderer_input_complex_color[3]->bitmap().pix16(y, cliprect.min_x);
    const u16 *s3attr  = &m_renderer_input_complex_attr[3] ->bitmap().pix16(y, cliprect.min_x);
    u16 *fc = &m_renderer_output_color[0]->bitmap().pix16(y, cliprect.min_x);
    u16 *fa = &m_renderer_output_attr[0] ->bitmap().pix16(y, cliprect.min_x);
    u16 *bc = &m_renderer_output_color[1]->bitmap().pix16(y, cliprect.min_x);
    u16 *ba = &m_renderer_output_attr[1] ->bitmap().pix16(y, cliprect.min_x);
    for(int x = cliprect.min_x; x <= cliprect.max_x; x++) {
      u16 prif = 0x7ff, prib = 0x7ff;
      u16 colorf = 0x0000, colorb = 0x0000, attrf = 0x0000, attrb = 0x0000;
      u16 oa = *oattr++;

      if(disp & 0x01) {
	u16 col = *acolor++ & 0x3ff;
	if(col & m_color_mask[0]) {
	  u16 lpri = (m_colchg_on & 8) && ((m_colpri[0] & m_color_mask[0]) <= (col & m_color_mask[0])) ? m_pri[8] : m_pri[0];
	  lpri = ((lpri << 3) | 0) ^ prixor;
	  prif = lpri;
	  colorf = (col & (0x0ff | (m_v_inmix_on << 8))) | (m_cblk[0] << 10);
	  attrf = m_shd_on & 0x01 ? 0x8001 : 0x8000;
	  attrf |= (m_v_inbri << 2) & 0xc;
	  attrf |= ((v_inmix_masked << 4) | ((~m_v_inmix_on << 4) & (col >> 4))) & 0x30;
	}
      }
				
      if(disp & 0x02) {
	u16 col = *bcolor++ & 0x3ff;
	if(col & m_color_mask[1]) {
	  u16 lpri = (m_colchg_on & 2) && ((m_colpri[1] & m_color_mask[1]) <= (col & m_color_mask[1])) ? m_pri[9] : m_pri[1];
	  lpri = ((lpri << 3) | 1) ^ prixor;

	  u16 ncolor = (col & (0x0ff | (m_v_inmix_on << 6))) | (m_cblk[1] << 10);
	  u16 nattr = m_shd_on & 0x02 ? 0x8001 : 0x8000;
	  nattr |= (m_v_inbri << 0) & 0xc;
	  nattr |= ((v_inmix_masked << 2) | ((~m_v_inmix_on << 2) & (col >> 4))) & 0x30;
	  ncolor = ncolor ;
	  if(!attrf || lpri < prif) {
	    prib = prif; colorb = colorf; attrb = attrf;
	    prif = lpri;
	    colorf = ncolor;
	    attrf = nattr;
	  } else {
	    prib = lpri;
	    colorb = ncolor;
	    attrb = nattr;
	  }
	}
      }

      if(disp & 0x04) {
	u16 col = *ccolor++ & 0x3ff;
	if(col & m_color_mask[2]) {
	  u16 lpri = m_pri[2];
	  lpri = ((lpri << 3) | 2) ^ prixor;

	  if(!attrb || lpri < prib) {
	    u16 ncolor = (col & (0x0ff | (m_v_inmix_on << 4))) | (m_cblk[2] << 10);
	    u16 nattr = m_shd_on & 0x04 ? 0x8001 : 0x8000;
	    nattr |= (m_v_inbri >> 2) & 0xc;
	    nattr |= ((v_inmix_masked << 0) | ((~m_v_inmix_on << 0) & (col >> 4))) & 0x30;
	    if(!attrf || lpri < prif) {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    } else {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	  }
	}
      }

      if(disp & 0x08) {
	u16 col = *dcolor++ & 0x3ff;
	if(col & m_color_mask[3]) {
	  u16 lpri = m_pri[3];
	  lpri = ((lpri << 3) | 3) ^ prixor;

	  if(!attrb || lpri < prib) {
	    u16 ncolor = (col & (0x0ff | (m_v_inmix_on << 2))) | (m_cblk[3] << 10);
	    u16 nattr = m_shd_on & 0x08 ? 0x8001 : 0x8000;
	    nattr |= (m_v_inbri >> 4) & 0xc;
	    nattr |= ((v_inmix_masked >> 2) | ((~m_v_inmix_on >> 2) & (col >> 4))) & 0x30;
	    if(!attrf || lpri < prif) {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    } else {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	  }
	}
      }

      if(disp & 0x10) {
	u16 col = *ocolor++ & 0xff;
	if(col & m_color_mask[4]) {
	  u16 lpri = ((oa & ~m_inpri_on[0]) | (m_pri[4] & m_inpri_on[0])) & 0xff;
	  lpri = ((lpri << 3) | 4) ^ prixor;

	  if(!attrb || lpri < prib) {
	    u16 ncolor = col | (((oa & m_inpri_on[0]) | ((m_cblk[4] << 2) & ~m_inpri_on[0])) << 8);
	    u16 nattr = m_shd_on & 0x10 ? 0x8001 : 0x8000;
	    nattr |= ((os_inbri_masked << 2) | ((oa >> 8) & ~(m_os_inbri_on << 2))) & 0xc;
	    nattr |= ((os_inmix_masked << 4) | ((~m_os_inmix_on << 4) & (oa >> 8))) & 0x30;
	    if(!attrf || lpri < prif) {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    } else {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	  }
	}
      }

      if(disp & 0x20) {
	u16 col  = *s1color++;
	u16 attr = *s1attr++;
	if(col & m_color_mask[5]) {
	  // Note that the order is O-S2-S3-S1, so the 7 is correct
	  u16 lpri = ((attr & ~m_inpri_on[1]) | (m_pri[5] & m_inpri_on[1])) & 0xff;
	  lpri = ((lpri << 3) | 7) ^ prixor;

	  if(!attrb || lpri < prib) {
	    u16 ncolor = col | (((attr & m_inpri_on[1]) | ((m_cblk[5] << 2) & ~m_inpri_on[1])) << 8);
	    u16 nattr = m_shd_on & 0x20 ? 0x8001 : 0x8000;
	    nattr |= ((os_inbri_masked) | ((attr >> 8) & ~(m_os_inbri_on))) & 0xc;
	    nattr |= ((os_inmix_masked << 2) | ((~m_os_inmix_on << 2) & (attr >> 8))) & 0x30;
	    if(!attrf || lpri < prif) {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    } else {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	  }
	}
      }

      if(disp & 0x40) {
	u16 col  = *s2color++;
	u16 attr = *s2attr++;
	if(col & m_color_mask[6]) {
	  u16 lpri = ((attr & ~m_inpri_on[2]) | (m_pri[6] & m_inpri_on[2])) & 0xff;
	  lpri = ((lpri << 3) | 5) ^ prixor;

	  if(!attrb || lpri < prib) {
	    u16 ncolor = (col & (0x3ff | (m_cblk_on[0] << 10))) | ((m_cblk[6] & ~m_cblk_on[0]) << 10);
	    u16 nattr = m_shd_on & 0x40 ? 0x8001 : 0x8000;
	    nattr |= ((os_inbri_masked >> 2) | ((attr >> 8) & ~(m_os_inbri_on >> 2))) & 0xc;
	    nattr |= ((os_inmix_masked) | ((~m_os_inmix_on) & (attr >> 8))) & 0x30;
	    if(!attrf || lpri < prif) {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    } else {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	  }
	}
      }

      if(disp & 0x80) {
	u16 col  = *s3color++;
	u16 attr = *s3attr++;
	if(col & m_color_mask[7]) {
	  u16 lpri = ((attr & ~m_inpri_on[3]) | (m_pri[7] & m_inpri_on[3])) & 0xff;
	  lpri = ((lpri << 3) | 6) ^ prixor;

	  if(!attrb || lpri < prib) {
	    u16 ncolor = (col & (0x3ff | (m_cblk_on[1] << 10))) | ((m_cblk[7] & ~m_cblk_on[1]) << 10);
	    u16 nattr = m_shd_on & 0x80 ? 0x8001 : 0x8000;
	    nattr |= ((os_inbri_masked >> 4) | ((attr >> 8) & ~(m_os_inbri_on >> 4))) & 0xc;
	    nattr |= ((os_inmix_masked >> 2) | ((~m_os_inmix_on >> 2) & (attr >> 8))) & 0x30;
	    if(!attrf || lpri < prif) {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    } else {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	  }
	}
      }

      if(attrf & 1)
	attrf ^= m_shadow_value[(oa >> 8) & 3][(prif ^ prixor) >> 3];
      if(attrb & 1)
	attrb ^= m_shadow_value[(oa >> 8) & 3][(prib ^ prixor) >> 3];

      if(!(attrf & attrb & 0x8000)) {
	u16 col = (m_bgc_cblk & 0xf) << 9;
	if(m_bgc_set & 0x0002) {
	  if(m_bgc_set & 0x0001)
	    col |= x & 0x1ff;
	  else
	    col |= y & 0x1ff;
	}

	if(!(attrf & 0x8000))
	  colorf = col;
	if(!(attrb & 0x8000))
	  colorb = col;
      }

      *fc++ = colorf;
      *fa++ = attrf;
      *bc++ = colorb;
      *ba++ = attrb;
    }
  }
}

void mixer::prepare_jit(jit::Context &jctxt)
{
  renderer = new jit::Function(jctxt);

  auto cliprect = renderer->mk_input<rectangle *>("cliprect");

  auto bgc_set = renderer->mk_param<u8>("bgc_set", this, &m_bgc_set);
  auto os_inbri = renderer->mk_param<u8>("os_inbri", this, &m_os_inbri);
  auto os_inbri_on = renderer->mk_param<u8>("os_inbri_on", this, &m_os_inbri_on);
  auto v_inbri = renderer->mk_param<u8>("v_inbri", this, &m_v_inbri);
  auto v_inmix = renderer->mk_param<u8>("v_inmix", this, &m_v_inmix);
  auto v_inmix_on = renderer->mk_param<u8>("v_inmix_on", this, &m_v_inmix_on);
  auto os_inmix = renderer->mk_param<u8>("os_inmix", this, &m_os_inmix);
  auto os_inmix_on = renderer->mk_param<u8>("os_inmix_on", this, &m_os_inmix_on);
  auto colchg_on = renderer->mk_param<u8>("colchg_on", this, &m_colchg_on);
  auto shd_on = renderer->mk_param<u8>("shd_on", this, &m_shd_on);

  auto color_mask = renderer->mk_param<u16, 8>("color_mask", this, &m_color_mask);
  auto colpri = renderer->mk_param<u8, 2>("colpri", this, &m_colpri);
  auto cblk = renderer->mk_param<u8, 8>("cblk", this, &m_cblk);
  auto cblk_on = renderer->mk_param<u8, 2>("cblk_on", this, &m_cblk_on);
  auto inpri_on = renderer->mk_param<u8, 4>("inpri_on", this, &m_inpri_on);
  auto bgc_cblk = renderer->mk_param<u8>("bgc_cblk", this, &m_bgc_cblk);

  auto pri = renderer->mk_extvar<u8, 8+2>("pri", this, &m_pri);

  auto prixor = renderer->mk_var<u16>("prixor", (bgc_set & 8).ternary(0x7ff, 0));
  auto shadow_value = renderer->mk_extvar<u8, 4, 256>("shadow_value", this, &m_shadow_value);

  auto os_inbri_masked = os_inbri & os_inbri_on;
  auto v_inmix_masked  = v_inmix  & v_inmix_on;
  auto os_inmix_masked = os_inmix & os_inmix_on;

  auto disp = renderer->mk_param<u8>("disp", this, &m_disp);

  auto y = renderer->range_loop<int>("y", cliprect.member<int>(offsetof(rectangle, min_y)), cliprect.member<int>(offsetof(rectangle, max_y)) + 1);
  {
    auto acolor  = m_renderer_input_simple_color[0] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto bcolor  = m_renderer_input_simple_color[1] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto ccolor  = m_renderer_input_simple_color[2] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto dcolor  = m_renderer_input_simple_color[3] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto ocolor  = m_renderer_input_complex_color[0]->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto oattr   = m_renderer_input_complex_attr[0] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto s1color = m_renderer_input_complex_color[1]->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto s1attr  = m_renderer_input_complex_attr[1] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto s2color = m_renderer_input_complex_color[2]->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto s2attr  = m_renderer_input_complex_attr[2] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto s3color = m_renderer_input_complex_color[3]->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto s3attr  = m_renderer_input_complex_attr[3] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto fc = m_renderer_output_color[0]->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto fa = m_renderer_output_attr[0] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto bc = m_renderer_output_color[1]->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto ba = m_renderer_output_attr[1] ->bitmap().jit_pix16(renderer, y, cliprect.member<int>(offsetof(rectangle, min_x)));
    auto x = renderer->range_loop<int>("x", cliprect.member<int>(offsetof(rectangle, min_x)), cliprect.member<int>(offsetof(rectangle, max_x)) + 1);
    {
      auto prif = renderer->mk_var<u16>("prif", 0x7ff);
      auto prib = renderer->mk_var<u16>("prib", 0x7ff);
      auto colorf = renderer->mk_var<u16>("colorf", 0);
      auto attrf = renderer->mk_var<u16>("attrf", 0);
      auto colorb = renderer->mk_var<u16>("colorb", 0);
      auto attrb = renderer->mk_var<u16>("attrb", 0);
      
      auto oa = *oattr++;

      renderer->jif(disp & 0x01);
      {
	auto col = *acolor++ & 0x3ff;
	renderer->jif(col & color_mask[0]);
	{
	  auto lpri = ((colchg_on & 8) && ((colpri[0] & color_mask[0]) <= (col & color_mask[0]))).ternary(pri[8], pri[0]);
	  lpri = ((lpri << 3) | 0) ^ prixor;
	  prif = lpri;
	  colorf = (col & (0x0ff | (v_inmix_on << 8))) | (m_cblk[0] << 10);
	  attrf = (shd_on & 0x01).ternary(0x8001, 0x8000);
	  attrf |= (v_inbri << 2) & 0xc;
	  attrf |= ((v_inmix_masked << 4) | ((~v_inmix_on << 4) & (col >> 4))) & 0x30;
	}
	renderer->end();
      }
      renderer->end();

      renderer->jif(disp & 0x02);
      {
	auto col = *bcolor++ & 0x3ff;
	renderer->jif(col & color_mask[1]);
	{
	  auto lpri = ((colchg_on & 2) && ((colpri[1] & color_mask[1]) <= (col & color_mask[1]))).ternary(pri[9], pri[1]);
	  lpri = ((lpri << 3) | 1) ^ prixor;

	  auto ncolor = (col & (0x0ff | (v_inmix_on << 6))) | (cblk[1] << 10);
	  auto nattr = (shd_on & 0x02).ternary(0x8001, 0x8000);
	  nattr |= (v_inbri << 0) & 0xc;
	  nattr |= ((v_inmix_masked << 2) | ((~v_inmix_on << 2) & (col >> 4))) & 0x30;
	  ncolor = ncolor ;
	  renderer->jif(!attrf || lpri < prif);
	  {
	    prib = prif; colorb = colorf; attrb = attrf;
	    prif = lpri;
	    colorf = ncolor;
	    attrf = nattr;
	  }
	  renderer->jelse();
	  {
	    prib = lpri;
	    colorb = ncolor;
	    attrb = nattr;
	  }
	  renderer->end();
	}
	renderer->end();
      }
      renderer->end();

      renderer->jif(disp & 0x04);
      {
	auto col = *ccolor++ & 0x3ff;
	renderer->jif(col & color_mask[2]);
	{
	  auto lpri = pri[2];
	  lpri = ((lpri << 3) | 2) ^ prixor;

	  renderer->jif(!attrb || lpri < prib);
	  {
	    auto ncolor = (col & (0x0ff | (v_inmix_on << 4))) | (cblk[2] << 10);
	    auto nattr = (shd_on & 0x04).ternary(0x8001, 0x8000);
	    nattr |= (v_inbri >> 2) & 0xc;
	    nattr |= ((v_inmix_masked << 0) | ((~v_inmix_on << 0) & (col >> 4))) & 0x30;
	    renderer->jif(!attrf || lpri < prif);
	    {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    }
	    renderer->jelse();
	    {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	    renderer->end();
	  }
	  renderer->end();
	}
	renderer->end();
      }
      renderer->end();

      renderer->jif(disp & 0x08);
      {
	auto col = *dcolor++ & 0x3ff;
	renderer->jif(col & color_mask[3]);
	{
	  auto lpri = pri[3];
	  lpri = ((lpri << 3) | 3) ^ prixor;

	  renderer->jif(!attrb || lpri < prib);
	  {
	    auto ncolor = (col & (0x0ff | (v_inmix_on << 2))) | (cblk[3] << 10);
	    auto nattr = (shd_on & 0x08).ternary(0x8001, 0x8000);
	    nattr |= (v_inbri >> 4) & 0xc;
	    nattr |= ((v_inmix_masked >> 2) | ((~v_inmix_on >> 2) & (col >> 4))) & 0x30;
	    renderer->jif(!attrf || lpri < prif);
	    {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    }
	    renderer->jelse();
	    {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	    renderer->end();
	  }
	  renderer->end();
	}
	renderer->end();
      }
      renderer->end();

      renderer->jif(disp & 0x10);
      {
	auto col = *ocolor++ & 0xff;
	renderer->jif(col & color_mask[4]);
	{
	  auto lpri = ((oa & ~inpri_on[0]) | (pri[4] & inpri_on[0])) & 0xff;
	  lpri = ((lpri << 3) | 4) ^ prixor;

	  renderer->jif(!attrb || lpri < prib);
	  {
	    auto ncolor = col | (((oa & inpri_on[0]) | ((cblk[4] << 2) & ~inpri_on[0])) << 8);
	    auto nattr = (shd_on & 0x10).ternary(0x8001, 0x8000);
	    nattr |= ((os_inbri_masked << 2) | ((oa >> 8) & ~(os_inbri_on << 2))) & 0xc;
	    nattr |= ((os_inmix_masked << 4) | ((~os_inmix_on << 4) & (oa >> 8))) & 0x30;
	    renderer->jif(!attrf || lpri < prif);
	    {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    }
	    renderer->jelse();
	    {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	    renderer->end();
	  }
	  renderer->end();
	}
	renderer->end();
      }
      renderer->end();

      renderer->jif(disp & 0x20);
      {
	auto col  = *s1color++;
	auto attr = *s1attr++;
	renderer->jif(col & color_mask[5]);
	{
	  // Note that the order is O-S2-S3-S1, so the 7 is correct
	  auto lpri = ((attr & ~inpri_on[1]) | (pri[5] & inpri_on[1])) & 0xff;
	  lpri = ((lpri << 3) | 7) ^ prixor;

	  renderer->jif(!attrb || lpri < prib);
	  {
	    auto ncolor = col | (((attr & inpri_on[1]) | ((cblk[5] << 2) & ~inpri_on[1])) << 8);
	    auto nattr = (shd_on & 0x20).ternary(0x8001, 0x8000);
	    nattr |= ((os_inbri_masked) | ((attr >> 8) & ~(os_inbri_on))) & 0xc;
	    nattr |= ((os_inmix_masked << 2) | ((~os_inmix_on << 2) & (attr >> 8))) & 0x30;
	    renderer->jif(!attrf || lpri < prif);
	    {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    }
	    renderer->jelse();
	    {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	    renderer->end();
	  }
	  renderer->end();
	}
	renderer->end();
      }
      renderer->end();

      renderer->jif(disp & 0x40);
      {
	auto col  = *s2color++;
	auto attr = *s2attr++;
	renderer->jif(col & color_mask[6]);
	{
	  auto lpri = ((attr & ~inpri_on[2]) | (pri[6] & inpri_on[2])) & 0xff;
	  lpri = ((lpri << 3) | 5) ^ prixor;

	  renderer->jif(!attrb || lpri < prib);
	  {
	    auto ncolor = (col & (0x3ff | (cblk_on[0] << 10))) | ((cblk[6] & ~cblk_on[0]) << 10);
	    auto nattr = (shd_on & 0x40).ternary(0x8001, 0x8000);
	    nattr |= ((os_inbri_masked >> 2) | ((attr >> 8) & ~(os_inbri_on >> 2))) & 0xc;
	    nattr |= ((os_inmix_masked) | ((~os_inmix_on) & (attr >> 8))) & 0x30;
	    renderer->jif(!attrf || lpri < prif);
	    {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    }
	    renderer->jelse();
	    {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	    renderer->end();
	  }
	  renderer->end();
	}
	renderer->end();
      }
      renderer->end();

      renderer->jif(disp & 0x80);
      {
	auto col  = *s3color++;
	auto attr = *s3attr++;
	renderer->jif(col & color_mask[7]);
	{
	  auto lpri = ((attr & ~inpri_on[3]) | (pri[7] & inpri_on[3])) & 0xff;
	  lpri = ((lpri << 3) | 6) ^ prixor;

	  renderer->jif(!attrb || lpri < prib);
	  {
	    auto ncolor = (col & (0x3ff | (cblk_on[1] << 10))) | ((cblk[7] & ~cblk_on[1]) << 10);
	    auto nattr = (shd_on & 0x80).ternary(0x8001, 0x8000);
	    nattr |= ((os_inbri_masked >> 4) | ((attr >> 8) & ~(os_inbri_on >> 4))) & 0xc;
	    nattr |= ((os_inmix_masked >> 2) | ((~os_inmix_on >> 2) & (attr >> 8))) & 0x30;
	    renderer->jif(!attrf || lpri < prif);
	    {
	      prib = prif; colorb = colorf; attrb = attrf;
	      prif = lpri;
	      colorf = ncolor;
	      attrf = nattr;
	    }
	    renderer->jelse();
	    {
	      prib = lpri;
	      colorb = ncolor;
	      attrb = nattr;
	    }
	    renderer->end();
	  }
	  renderer->end();
	}
	renderer->end();
      }
      renderer->end();

      renderer->jif(attrf & 1);
      {
	attrf ^= shadow_value[(oa >> 8) & 3][(prif ^ prixor) >> 3];
      }
      renderer->end();

      renderer->jif(attrb & 1);
      {
	attrb ^= shadow_value[(oa >> 8) & 3][(prib ^ prixor) >> 3];
      }
      renderer->end();

      renderer->jif(!(attrf & attrb & 0x8000));
      {
	auto col = (bgc_cblk & 0xf) << 9;
	renderer->jif(bgc_set & 0x0002);
	{
	  renderer->jif(bgc_set & 0x0001);
	  {
	    col |= x & 0x1ff;
	  }
	  renderer->jelse();
	  {
	    col |= y & 0x1ff;
	  }
	  renderer->end();
	}
	renderer->end();

	renderer->jif(!(attrf & 0x8000));
	{
	  colorf = col;
	}
	renderer->end();

	renderer->jif(!(attrb & 0x8000));
	{
	  colorb = col;
	}
	renderer->end();

      }
      renderer->end();

      *fc++ = colorf;
      *fa++ = attrf;
      *bc++ = colorb;
      *ba++ = attrb;
    }
    renderer->end();
  }
  renderer->end();
}

void mixer::render_jit(rectangle &cliprect)
{
}

int main()
{
  mixer m;
  rectangle cliprect(0, 0, 319, 223);
  m.render_default(cliprect);
  jit::Context jctxt;
  m.prepare_jit(jctxt);
  m.render_jit(cliprect);
  return 0;
}
