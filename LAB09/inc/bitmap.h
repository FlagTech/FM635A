// 'banner_background', 128x16px
const unsigned char bitmap_banner_background [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
// 'flash', 44x44px
const unsigned char bitmap_flash [] PROGMEM = {
	0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf8, 0x00, 
	0x00, 0x00, 0x00, 0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x03, 0xff, 
	0xfe, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0x00, 0x00, 0x00, 
	0x7f, 0xff, 0xff, 0x80, 0x00, 0x00, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0xff, 0xff, 0xff, 0xc0, 
	0x00, 0x00, 0x7f, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x0f, 0xff, 
	0xff, 0xf0, 0x00, 0x00, 0x07, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x01, 0xff, 0xff, 0xe0, 0x00, 0x00, 
	0x00, 0x7f, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfc, 
	0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0x00, 0x00, 0x00, 0x3f, 
	0xff, 0xff, 0x80, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x07, 0xff, 0xff, 0xe0, 0x00, 
	0x00, 0x03, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x3f, 0xff, 
	0x80, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0x00, 0x00, 0x00, 
	0x3f, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xf8, 0x00, 
	0x00, 0x00, 0x07, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x7f, 
	0xfe, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x80, 0x00, 0x00, 
	0x00, 0x03, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xe0, 
	0x00, 0x00, 0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x00, 0x00, 
	0x03, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0
};
// 'heart', 44x44px
const unsigned char bitmap_heart [] PROGMEM = {
	0x00, 0x7c, 0x00, 0x03, 0xe0, 0x00, 0x03, 0xff, 0x00, 0x0f, 0xfc, 0x00, 0x07, 0xff, 0xc0, 0x3f, 
	0xfe, 0x00, 0x0f, 0xff, 0xe0, 0x7f, 0xff, 0x00, 0x1f, 0xff, 0xf0, 0xff, 0xff, 0x80, 0x3f, 0xff, 
	0xf9, 0xff, 0xff, 0xc0, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xe0, 
	0x7f, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xff, 
	0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x7f, 0xff, 0xff, 0xff, 
	0xff, 0xe0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x7f, 0xff, 
	0xff, 0xff, 0xff, 0xe0, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xc0, 
	0x1f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 0xff, 0xff, 0x80, 0x1f, 0xff, 0xff, 0xff, 
	0xff, 0x80, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x07, 0xff, 
	0xff, 0xff, 0xfe, 0x00, 0x03, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x01, 0xff, 0xff, 0xff, 0xfc, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x7f, 0xff, 0xff, 
	0xe0, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x1f, 0xff, 0xff, 0x80, 0x00, 0x00, 0x0f, 
	0xff, 0xff, 0x00, 0x00, 0x00, 0x07, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x03, 0xff, 0xfc, 0x00, 0x00, 
	0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00
};
// 'left_btn', 16x27px
const unsigned char bitmap_left_btn [] PROGMEM = {
	0x00, 0x0e, 0x00, 0x1f, 0x00, 0x3f, 0x00, 0x7f, 0x00, 0xfe, 0x01, 0xfc, 0x03, 0xf8, 0x07, 0xf0, 
	0x0f, 0xe0, 0x1f, 0xc0, 0x3f, 0x80, 0x7f, 0x00, 0xfe, 0x00, 0xfc, 0x00, 0xfe, 0x00, 0x7f, 0x00, 
	0x3f, 0x80, 0x1f, 0xc0, 0x0f, 0xe0, 0x07, 0xf0, 0x03, 0xf8, 0x01, 0xfc, 0x00, 0xfe, 0x00, 0x7f, 
	0x00, 0x3f, 0x00, 0x1f, 0x00, 0x0e
};
// 'right_btn', 16x27px
const unsigned char bitmap_right_btn [] PROGMEM = {
	0x70, 0x00, 0xf8, 0x00, 0xfc, 0x00, 0xfe, 0x00, 0x7f, 0x00, 0x3f, 0x80, 0x1f, 0xc0, 0x0f, 0xe0, 
	0x07, 0xf0, 0x03, 0xf8, 0x01, 0xfc, 0x00, 0xfe, 0x00, 0x7f, 0x00, 0x3f, 0x00, 0x7f, 0x00, 0xfe, 
	0x01, 0xfc, 0x03, 0xf8, 0x07, 0xf0, 0x0f, 0xe0, 0x1f, 0xc0, 0x3f, 0x80, 0x7f, 0x00, 0xfe, 0x00, 
	0xfc, 0x00, 0xf8, 0x00, 0x70, 0x00
};
// 'star', 44x44px
const unsigned char bitmap_star [] PROGMEM = {
	0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 
	0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 
	0x3f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 
	0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xe0, 
	0x00, 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 
	0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xc0, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x1f, 0xff, 
	0xff, 0xff, 0xff, 0x80, 0x07, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x03, 0xff, 0xff, 0xff, 0xfc, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x3f, 0xff, 0xff, 
	0xc0, 0x00, 0x00, 0x1f, 0xff, 0xff, 0x80, 0x00, 0x00, 0x1f, 0xff, 0xff, 0x80, 0x00, 0x00, 0x1f, 
	0xff, 0xff, 0x80, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xc0, 0x00, 
	0x00, 0x3f, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x7f, 0xff, 0xff, 
	0xe0, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x7f, 0xf9, 0xff, 0xe0, 0x00, 0x00, 0x7f, 
	0xf0, 0xff, 0xf0, 0x00, 0x00, 0xff, 0xe0, 0x3f, 0xf0, 0x00, 0x00, 0xff, 0x80, 0x1f, 0xf0, 0x00, 
	0x00, 0xff, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0xfc, 0x00, 0x03, 0xf0, 0x00, 0x00, 0xf8, 0x00, 0x01, 
	0xf0, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x70, 0x00
};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 1296)
const int bitmap_allArray_LEN = 6;
const unsigned char* bitmap_allArray[6] = {
	bitmap_banner_background,
	bitmap_flash,
	bitmap_heart,
	bitmap_left_btn,
	bitmap_right_btn,
	bitmap_star
};
