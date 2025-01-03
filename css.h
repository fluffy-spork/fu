#pragma once

#define PROPERTY_ENUM_CSS(var, E) \
    E(appearance, "appearance", var) \
    E(background_color, "background-color", var) \
    E(border, "border", var) \
    E(border_bottom, "border-bottom", var) \
    E(border_bottom_left_radius, "border-bottom-left-radius", var) \
    E(border_collapse, "border-collapse", var) \
    E(border_color, "border-color", var) \
    E(border_left, "border-left", var) \
    E(border_right, "border-right", var) \
    E(border_top, "border-top", var) \
    E(border_radius, "border-radius", var) \
    E(border_spacing, "border-spacing", var) \
    E(border_style, "border-style", var) \
    E(bottom, "bottom", var) \
    E(box_shadow, "box-shadow", var) \
    E(box_sizing, "box-sizing", var) \
    E(clip, "clip", var) \
    E(color, "color", var) \
    E(cursor, "cursor", var) \
    E(display, "display", var) \
    E(flex_direction, "flex-direction", var) \
    E(flex_grow, "flex-grow", var) \
    E(flex_wrap, "flex-wrap", var) \
    E(float_pos, "float", var) \
    E(font, "font", var) \
    E(font_family, "font-family", var) \
    E(font_size, "font-size", var) \
    E(font_weight, "font-weight", var) \
    E(height, "height", var) \
    E(justify_content, "justify-content", var) \
    E(left, "left", var) \
    E(line_height, "line-height", var) \
    E(margin, "margin", var) \
    E(margin_top, "margin-top", var) \
    E(margin_right, "margin-right", var) \
    E(margin_bottom, "margin-bottom", var) \
    E(margin_left, "margin-left", var) \
    E(max_width, "max-width", var) \
    E(max_height, "max-height", var) \
    E(min_width, "min-width", var) \
    E(min_height, "min-height", var) \
    E(object_fit, "object-fit", var) \
    E(object_position, "object-position", var) \
    E(outline, "outline", var) \
    E(overflow, "overflow", var) \
    E(padding, "padding", var) \
    E(pointer_events, "pointer-events", var) \
    E(position, "position", var) \
    E(right, "right", var) \
    E(text_align, "text-align", var) \
    E(text_decoration, "text-decoration", var) \
    E(text_overflow, "text-overflow", var) \
    E(text_shadow, "text-shadow", var) \
    E(text_wrap, "text-wrap", var) \
    E(top, "top", var) \
    E(touch_action, "touch-action", var) \
    E(transform, "transform", var) \
    E(user_select, "user-select", var) \
    E(vertical_align, "vertical-align", var) \
    E(visibility, "visibility", var) \
    E(white_space, "white-space", var) \
    E(width, "width", var) \
    E(x_overflow, "overflow-x", var) \
    E(y_overflow, "overflow-y", var) \
    E(z_index, "z-index", var) \


ENUM_BLOB(property_css_e, PROPERTY_ENUM_CSS);


#define GEN_PROP_FUNC_CSS(name, value, var_name) \
    void \
    name##_css(blob_t * css, const blob_t * prop_value) \
    { \
        add_blob(css, B(value)); \
        write_blob(css, ":", 1); \
        add_blob(css, prop_value); \
        write_blob(css, ";", 1); \
    } \


PROPERTY_ENUM_CSS(ignore, GEN_PROP_FUNC_CSS);



void
begin_css(blob_t * css, const blob_t * selector)
{
    add_blob(css, selector);
    write_blob(css, "{", 1);
}


void
tag_css(blob_t * css, const blob_t * tag)
{
    add_blob(css, tag);
    write_blob(css, "{", 1);
}


void
id_css(blob_t * css, const blob_t * id)
{
    write_blob(css, "#", 1);
    add_blob(css, id);
    write_blob(css, "{", 1);
}


void
class_css(blob_t * css, const blob_t * name)
{
    write_blob(css, ".", 1);
    add_blob(css, name);
    write_blob(css, "{", 1);
}


void
end_css(blob_t * css) {
    write_blob(css, "}", 1);
}


void
media_css(blob_t * css, blob_t * selector)
{
    add_blob(css, B("@media "));
    add_blob(css, selector);
    write_blob(css, "{", 1);
}


void
end_media_css(blob_t * css)
{
    end_css(css);
}


void
prop_css(blob_t * css, const blob_t * name, const blob_t * value)
{
    add_blob(css, name);
    write_blob(css, ":", 1);
    add_blob(css, value);
    write_blob(css, ";", 1);
}


void
link_css(blob_t * css, const blob_t * fg, const blob_t * bg)
{
    begin_css(css, B("a, a:link, a:visited"));
    {
        color_css(css, fg);
        text_shadow_css(css, B("0 0 2px #000"));
        text_decoration_css(css, B("none"));
    }
    end_css(css);

    begin_css(css, B("a:focus, a:hover"));
    {
        outline_css(css, B("none"));
        text_decoration_css(css, B("underline"));
    }
    end_css(css);

    begin_css(css, B("a:active"));
    {
        background_color_css(css, fg);
        color_css(css, bg);
        text_shadow_css(css, B("none"));
    }
    end_css(css);
}


// TODO(jason): good idea?
void
absolute_position_css(blob_t * css)
{
    position_css(css, B("absolute"));
}


// TODO(jason): good idea?
void
fixed_position_css(blob_t * css)
{
    position_css(css, B("fixed"));
}


// TODO(jason): good idea?
void
relative_position_css(blob_t * css)
{
    position_css(css, B("relative"));
}


void
init_css(void)
{
    init_property_css_e();
}


int
reset_css(blob_t * css)
{
    begin_css(css, B("*"));
    {
        margin_css(css, B("0"));
        padding_css(css, B("0"));
        border_css(css, B("0"));
    }
    end_css(css);

    return 0;
}

