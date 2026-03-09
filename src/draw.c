#include "../include/draw.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <wayland-client.h>

static void render_glyphs(
  pixman_image_t *result,
  int *x,
  const int *y,
  bool anchor_right,
  pixman_image_t *color,
  size_t count,
  struct fcft_font *font,
  const struct fcft_glyph *glyphs[]
) {
  if (anchor_right) { // places the pen at the correct spot
    size_t text_width = 0;
    for (size_t i = 0; i < count; i++) {
      text_width += glyphs[i]->advance.x;
    }

    *x -= text_width;
  }

  for (size_t i = 0; i < count; i++) {
    const struct fcft_glyph *g = glyphs[i];
    if (g == NULL)
      continue;

    if (g->is_color_glyph) {
      pixman_image_composite32(PIXMAN_OP_OVER, g->pix, NULL, result, 0, 0, 0, 0,
                               *x + g->x, *y + font->ascent - g->y, g->width,
                               g->height);
    } else {
      pixman_image_composite32(PIXMAN_OP_OVER, color, g->pix, result, 0, 0, 0,
                               0, *x + g->x, *y + font->ascent - g->y, g->width,
                               g->height);
    }
    *x += g->advance.x;
  }
}

void render_chars(
  const char32_t *text,
  pixman_image_t *result,
  size_t text_len,
  int *x,
  int y,
  int surface_width,
  bool anchor_right,
  pixman_image_t *color,
  struct fcft_font *font
) {
  if (text_len == 0) return;
  const struct fcft_glyph **glyphs = safe_malloc(text_len * sizeof(const struct fcft_glyph *));

  int text_width = 0;
  int avail_width = anchor_right ? *x : (surface_width - *x);

  // TODO: maybe use fcft_rasterize_text_run_utf32?
  for (size_t i = 0; i < text_len; i++) {
    glyphs[i] = fcft_rasterize_char_utf32(font, text[i], FCFT_SUBPIXEL_NONE);
    if (glyphs[i] == NULL)
      continue;

    text_width += glyphs[i]->advance.x;
    if (text_width > avail_width) {
      text_width -= glyphs[i]->advance.x;
      text_len = i;
      break;
    }
  }

  render_glyphs(result, x, &y, anchor_right, color, text_len, font, glyphs);
  free(glyphs);
}

void render_rows(OutputState *hud) {
  MeowhudState *state = hud->global_state;
  Row **rows = state->content_rows;
  int y = 0;
  for (size_t i = 0; i < state->row_count; i++) {
    Row *curr_row = rows[i];
    int left_x = 0;
    int right_x = hud->width;

    if (!curr_row) {
      continue; 
    }

    Text *curr_text;
    // renders left part of row
    if ((curr_row->left) != NULL) {
      curr_text = curr_row->left;
      for (size_t j = 0; j < curr_row->left->section_count; j++) {
        TextSection curr_section = curr_text->sections[j];
        if (curr_section.color != NULL) {
          render_chars(curr_section.text, hud->pix_img, curr_section.len,
                       &left_x, y, hud->width, false, curr_section.color,
                       state->font);
        } else { // use default color
          render_chars(curr_section.text, hud->pix_img, curr_section.len,
                       &left_x, y, hud->width, false, state->default_text_color,
                       state->font);
        }
      }
    }

    // renders right part of row
   if ((curr_row->right)) {
      curr_text = curr_row->right;
      for (size_t j = 0; j < curr_row->right->section_count; j++) {
        TextSection curr_section = curr_text->sections[j];
        if (curr_section.color != NULL) { 
          render_chars(curr_section.text, hud->pix_img, curr_section.len,
                       &right_x, y, hud->width, true, curr_section.color,
                       state->font);
        } else { // use default color
          render_chars(curr_section.text, hud->pix_img, curr_section.len,
                       &right_x, y, hud->width, true, state->default_text_color,
                       state->font);
        }
      }
    }

    y += state->font->height; // advance to the y of the next line
    y += state->row_spacing;
  }
}

void render_bg(OutputState *hud) {
  pixman_image_composite32(PIXMAN_OP_SRC, hud->global_state->bg_color, NULL,
                           hud->pix_img, 0, 0, 0, 0, 0, 0, hud->width,
                           hud->height);
}

void draw_hud(OutputState *hud) {
  wl_surface_attach(hud->surface, hud->buff, 0, 0);
  wl_surface_damage(hud->surface, 0, 0, hud->width, hud->height);
  wl_surface_commit(hud->surface);
  wl_display_flush(hud->global_state->display);
}

