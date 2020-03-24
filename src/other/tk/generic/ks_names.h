/*
 * This file should be maintained in sync with xlib/X11/keysymdefs.h
 *
 * Note that this should be done manually only, because in some cases
 * keysymdefs.h defines the same integer for multiple keysyms, e.g.:
 *
 *    #define XK_Greek_LAMDA                         0x7CB
 *    #define XK_Greek_LAMBDA                        0x7CB
 *
 *    #define XK_Cyrillic_DZHE                       0x6BF
 *    #define XK_Serbian_DZE                         0x6BF  (deprecated)
 *
 */
{ "BackSpace", 0xFF08 },
{ "Tab", 0xFF09 },
{ "Linefeed", 0xFF0A },
{ "Clear", 0xFF0B },
{ "Return", 0xFF0D },
{ "Pause", 0xFF13 },
{ "Scroll_Lock", 0xFF14 },
{ "Sys_Req", 0xFF15 },
{ "Escape", 0xFF1B },
{ "Delete", 0xFFFF },
{ "Multi_key", 0xFF20 },
{ "Kanji", 0xFF21 },
{ "Muhenkan", 0xFF22 },
{ "Henkan_Mode", 0xFF23 },
{ "Henkan", 0xFF23 },
{ "Romaji", 0xFF24 },
{ "Hiragana", 0xFF25 },
{ "Katakana", 0xFF26 },
{ "Hiragana_Katakana", 0xFF27 },
{ "Zenkaku", 0xFF28 },
{ "Hankaku", 0xFF29 },
{ "Zenkaku_Hankaku", 0xFF2A },
{ "Touroku", 0xFF2B },
{ "Massyo", 0xFF2C },
{ "Kana_Lock", 0xFF2D },
{ "Kana_Shift", 0xFF2E },
{ "Eisu_Shift", 0xFF2F },
{ "Eisu_toggle", 0xFF30 },
{ "Home", 0xFF50 },
{ "Left", 0xFF51 },
{ "Up", 0xFF52 },
{ "Right", 0xFF53 },
{ "Down", 0xFF54 },
{ "Prior", 0xFF55 },
{ "Page_Up", 0xFF55 },
{ "Next", 0xFF56 },
{ "Page_Down", 0xFF56 },
{ "End", 0xFF57 },
{ "Begin", 0xFF58 },
{ "Win_L", 0xFF5B },
{ "Win_R", 0xFF5C },
{ "App", 0xFF5D },
{ "Select", 0xFF60 },
{ "Print", 0xFF61 },
{ "Execute", 0xFF62 },
{ "Insert", 0xFF63 },
{ "Undo", 0xFF65 },
{ "Redo", 0xFF66 },
{ "Menu", 0xFF67 },
{ "Find", 0xFF68 },
{ "Cancel", 0xFF69 },
{ "Help", 0xFF6A },
{ "Break", 0xFF6B },
{ "Mode_switch", 0xFF7E },
{ "script_switch", 0xFF7E },
{ "Num_Lock", 0xFF7F },
{ "KP_Space", 0xFF80 },
{ "KP_Tab", 0xFF89 },
{ "KP_Enter", 0xFF8D },
{ "KP_F1", 0xFF91 },
{ "KP_F2", 0xFF92 },
{ "KP_F3", 0xFF93 },
{ "KP_F4", 0xFF94 },
{ "KP_Home", 0xFF95 },
{ "KP_Left", 0xFF96 },
{ "KP_Up", 0xFF97 },
{ "KP_Right", 0xFF98 },
{ "KP_Down", 0xFF99 },
{ "KP_Prior", 0xFF9A },
{ "KP_Page_Up", 0xFF9A },
{ "KP_Next", 0xFF9B },
{ "KP_Page_Down", 0xFF9B },
{ "KP_End", 0xFF9C },
{ "KP_Begin", 0xFF9D },
{ "KP_Insert", 0xFF9E },
{ "KP_Delete", 0xFF9F },
{ "KP_Equal", 0xFFBD },
{ "KP_Multiply", 0xFFAA },
{ "KP_Add", 0xFFAB },
{ "KP_Separator", 0xFFAC },
{ "KP_Subtract", 0xFFAD },
{ "KP_Decimal", 0xFFAE },
{ "KP_Divide", 0xFFAF },
{ "KP_0", 0xFFB0 },
{ "KP_1", 0xFFB1 },
{ "KP_2", 0xFFB2 },
{ "KP_3", 0xFFB3 },
{ "KP_4", 0xFFB4 },
{ "KP_5", 0xFFB5 },
{ "KP_6", 0xFFB6 },
{ "KP_7", 0xFFB7 },
{ "KP_8", 0xFFB8 },
{ "KP_9", 0xFFB9 },
{ "F1", 0xFFBE },
{ "F2", 0xFFBF },
{ "F3", 0xFFC0 },
{ "F4", 0xFFC1 },
{ "F5", 0xFFC2 },
{ "F6", 0xFFC3 },
{ "F7", 0xFFC4 },
{ "F8", 0xFFC5 },
{ "F9", 0xFFC6 },
{ "F10", 0xFFC7 },
{ "F11", 0xFFC8 },
{ "L1", 0xFFC8 },
{ "F12", 0xFFC9 },
{ "L2", 0xFFC9 },
{ "F13", 0xFFCA },
{ "L3", 0xFFCA },
{ "F14", 0xFFCB },
{ "L4", 0xFFCB },
{ "F15", 0xFFCC },
{ "L5", 0xFFCC },
{ "F16", 0xFFCD },
{ "L6", 0xFFCD },
{ "F17", 0xFFCE },
{ "L7", 0xFFCE },
{ "F18", 0xFFCF },
{ "L8", 0xFFCF },
{ "F19", 0xFFD0 },
{ "L9", 0xFFD0 },
{ "F20", 0xFFD1 },
{ "L10", 0xFFD1 },
{ "F21", 0xFFD2 },
{ "R1", 0xFFD2 },
{ "F22", 0xFFD3 },
{ "R2", 0xFFD3 },
{ "F23", 0xFFD4 },
{ "R3", 0xFFD4 },
{ "F24", 0xFFD5 },
{ "R4", 0xFFD5 },
{ "F25", 0xFFD6 },
{ "R5", 0xFFD6 },
{ "F26", 0xFFD7 },
{ "R6", 0xFFD7 },
{ "F27", 0xFFD8 },
{ "R7", 0xFFD8 },
{ "F28", 0xFFD9 },
{ "R8", 0xFFD9 },
{ "F29", 0xFFDA },
{ "R9", 0xFFDA },
{ "F30", 0xFFDB },
{ "R10", 0xFFDB },
{ "F31", 0xFFDC },
{ "R11", 0xFFDC },
{ "F32", 0xFFDD },
{ "R12", 0xFFDD },
{ "F33", 0xFFDE },
{ "R13", 0xFFDE },
{ "F34", 0xFFDF },
{ "R14", 0xFFDF },
{ "F35", 0xFFE0 },
{ "R15", 0xFFE0 },
{ "Shift_L", 0xFFE1 },
{ "Shift_R", 0xFFE2 },
{ "Control_L", 0xFFE3 },
{ "Control_R", 0xFFE4 },
{ "Caps_Lock", 0xFFE5 },
{ "Shift_Lock", 0xFFE6 },
{ "Meta_L", 0xFFE7 },
{ "Meta_R", 0xFFE8 },
{ "Alt_L", 0xFFE9 },
{ "Alt_R", 0xFFEA },
{ "Super_L", 0xFFEB },
{ "Super_R", 0xFFEC },
{ "Hyper_L", 0xFFED },
{ "Hyper_R", 0xFFEE },
{ "space", 0x20 },
{ "exclam", 0x21 },
{ "quotedbl", 0x22 },
{ "numbersign", 0x23 },
{ "dollar", 0x24 },
{ "percent", 0x25 },
{ "ampersand", 0x26 },
{ "apostrophe", 0x27 },
{ "quoteright", 0x27 },
{ "parenleft", 0x28 },
{ "parenright", 0x29 },
{ "asterisk", 0x2A },
{ "plus", 0x2B },
{ "comma", 0x2C },
{ "minus", 0x2D },
{ "period", 0x2E },
{ "slash", 0x2F },
{ "0", 0x30 },
{ "1", 0x31 },
{ "2", 0x32 },
{ "3", 0x33 },
{ "4", 0x34 },
{ "5", 0x35 },
{ "6", 0x36 },
{ "7", 0x37 },
{ "8", 0x38 },
{ "9", 0x39 },
{ "colon", 0x3A },
{ "semicolon", 0x3B },
{ "less", 0x3C },
{ "equal", 0x3D },
{ "greater", 0x3E },
{ "question", 0x3F },
{ "at", 0x40 },
{ "A", 0x41 },
{ "B", 0x42 },
{ "C", 0x43 },
{ "D", 0x44 },
{ "E", 0x45 },
{ "F", 0x46 },
{ "G", 0x47 },
{ "H", 0x48 },
{ "I", 0x49 },
{ "J", 0x4A },
{ "K", 0x4B },
{ "L", 0x4C },
{ "M", 0x4D },
{ "N", 0x4E },
{ "O", 0x4F },
{ "P", 0x50 },
{ "Q", 0x51 },
{ "R", 0x52 },
{ "S", 0x53 },
{ "T", 0x54 },
{ "U", 0x55 },
{ "V", 0x56 },
{ "W", 0x57 },
{ "X", 0x58 },
{ "Y", 0x59 },
{ "Z", 0x5A },
{ "bracketleft", 0x5B },
{ "backslash", 0x5C },
{ "bracketright", 0x5D },
{ "asciicircum", 0x5E },
{ "underscore", 0x5F },
{ "grave", 0x60 },
{ "quoteleft", 0x60 },
{ "a", 0x61 },
{ "b", 0x62 },
{ "c", 0x63 },
{ "d", 0x64 },
{ "e", 0x65 },
{ "f", 0x66 },
{ "g", 0x67 },
{ "h", 0x68 },
{ "i", 0x69 },
{ "j", 0x6A },
{ "k", 0x6B },
{ "l", 0x6C },
{ "m", 0x6D },
{ "n", 0x6E },
{ "o", 0x6F },
{ "p", 0x70 },
{ "q", 0x71 },
{ "r", 0x72 },
{ "s", 0x73 },
{ "t", 0x74 },
{ "u", 0x75 },
{ "v", 0x76 },
{ "w", 0x77 },
{ "x", 0x78 },
{ "y", 0x79 },
{ "z", 0x7A },
{ "braceleft", 0x7B },
{ "bar", 0x7C },
{ "braceright", 0x7D },
{ "asciitilde", 0x7E },
{ "nobreakspace", 0xA0 },
{ "exclamdown", 0xA1 },
{ "cent", 0xA2 },
{ "sterling", 0xA3 },
{ "currency", 0xA4 },
{ "yen", 0xA5 },
{ "brokenbar", 0xA6 },
{ "section", 0xA7 },
{ "diaeresis", 0xA8 },
{ "copyright", 0xA9 },
{ "ordfeminine", 0xAA },
{ "guillemotleft", 0xAB },
{ "notsign", 0xAC },
{ "hyphen", 0xAD },
{ "registered", 0xAE },
{ "macron", 0xAF },
{ "degree", 0xB0 },
{ "plusminus", 0xB1 },
{ "twosuperior", 0xB2 },
{ "threesuperior", 0xB3 },
{ "acute", 0xB4 },
{ "mu", 0xB5 },
{ "paragraph", 0xB6 },
{ "periodcentered", 0xB7 },
{ "cedilla", 0xB8 },
{ "onesuperior", 0xB9 },
{ "masculine", 0xBA },
{ "guillemotright", 0xBB },
{ "onequarter", 0xBC },
{ "onehalf", 0xBD },
{ "threequarters", 0xBE },
{ "questiondown", 0xBF },
{ "Agrave", 0xC0 },
{ "Aacute", 0xC1 },
{ "Acircumflex", 0xC2 },
{ "Atilde", 0xC3 },
{ "Adiaeresis", 0xC4 },
{ "Aring", 0xC5 },
{ "AE", 0xC6 },
{ "Ccedilla", 0xC7 },
{ "Egrave", 0xC8 },
{ "Eacute", 0xC9 },
{ "Ecircumflex", 0xCA },
{ "Ediaeresis", 0xCB },
{ "Igrave", 0xCC },
{ "Iacute", 0xCD },
{ "Icircumflex", 0xCE },
{ "Idiaeresis", 0xCF },
{ "ETH", 0xD0 },
{ "Eth", 0xD0 },
{ "Ntilde", 0xD1 },
{ "Ograve", 0xD2 },
{ "Oacute", 0xD3 },
{ "Ocircumflex", 0xD4 },
{ "Otilde", 0xD5 },
{ "Odiaeresis", 0xD6 },
{ "multiply", 0xD7 },
{ "Oslash", 0xD8 },
{ "Ooblique", 0xD8 },
{ "Ugrave", 0xD9 },
{ "Uacute", 0xDA },
{ "Ucircumflex", 0xDB },
{ "Udiaeresis", 0xDC },
{ "Yacute", 0xDD },
{ "THORN", 0xDE },
{ "Thorn", 0xDE },
{ "ssharp", 0xDF },
{ "agrave", 0xE0 },
{ "aacute", 0xE1 },
{ "acircumflex", 0xE2 },
{ "atilde", 0xE3 },
{ "adiaeresis", 0xE4 },
{ "aring", 0xE5 },
{ "ae", 0xE6 },
{ "ccedilla", 0xE7 },
{ "egrave", 0xE8 },
{ "eacute", 0xE9 },
{ "ecircumflex", 0xEA },
{ "ediaeresis", 0xEB },
{ "igrave", 0xEC },
{ "iacute", 0xED },
{ "icircumflex", 0xEE },
{ "idiaeresis", 0xEF },
{ "eth", 0xF0 },
{ "ntilde", 0xF1 },
{ "ograve", 0xF2 },
{ "oacute", 0xF3 },
{ "ocircumflex", 0xF4 },
{ "otilde", 0xF5 },
{ "odiaeresis", 0xF6 },
{ "division", 0xF7 },
{ "oslash", 0xF8 },
{ "ugrave", 0xF9 },
{ "uacute", 0xFA },
{ "ucircumflex", 0xFB },
{ "udiaeresis", 0xFC },
{ "yacute", 0xFD },
{ "thorn", 0xFE },
{ "ydiaeresis", 0xFF },
{ "Aogonek", 0x1A1 },
{ "breve", 0x1A2 },
{ "Lstroke", 0x1A3 },
{ "Lcaron", 0x1A5 },
{ "Sacute", 0x1A6 },
{ "Scaron", 0x1A9 },
{ "Scedilla", 0x1AA },
{ "Tcaron", 0x1AB },
{ "Zacute", 0x1AC },
{ "Zcaron", 0x1AE },
{ "Zabovedot", 0x1AF },
{ "aogonek", 0x1B1 },
{ "ogonek", 0x1B2 },
{ "lstroke", 0x1B3 },
{ "lcaron", 0x1B5 },
{ "sacute", 0x1B6 },
{ "caron", 0x1B7 },
{ "scaron", 0x1B9 },
{ "scedilla", 0x1BA },
{ "tcaron", 0x1BB },
{ "zacute", 0x1BC },
{ "doubleacute", 0x1BD },
{ "zcaron", 0x1BE },
{ "zabovedot", 0x1BF },
{ "Racute", 0x1C0 },
{ "Abreve", 0x1C3 },
{ "Lacute", 0x1C5 },
{ "Cacute", 0x1C6 },
{ "Ccaron", 0x1C8 },
{ "Eogonek", 0x1CA },
{ "Ecaron", 0x1CC },
{ "Dcaron", 0x1CF },
{ "Dstroke", 0x1D0 },
{ "Nacute", 0x1D1 },
{ "Ncaron", 0x1D2 },
{ "Odoubleacute", 0x1D5 },
{ "Rcaron", 0x1D8 },
{ "Uring", 0x1D9 },
{ "Udoubleacute", 0x1DB },
{ "Tcedilla", 0x1DE },
{ "racute", 0x1E0 },
{ "abreve", 0x1E3 },
{ "lacute", 0x1E5 },
{ "cacute", 0x1E6 },
{ "ccaron", 0x1E8 },
{ "eogonek", 0x1EA },
{ "ecaron", 0x1EC },
{ "dcaron", 0x1EF },
{ "dstroke", 0x1F0 },
{ "nacute", 0x1F1 },
{ "ncaron", 0x1F2 },
{ "odoubleacute", 0x1F5 },
{ "rcaron", 0x1F8 },
{ "uring", 0x1F9 },
{ "udoubleacute", 0x1FB },
{ "tcedilla", 0x1FE },
{ "abovedot", 0x1FF },
{ "Hstroke", 0x2A1 },
{ "Hcircumflex", 0x2A6 },
{ "Iabovedot", 0x2A9 },
{ "Gbreve", 0x2AB },
{ "Jcircumflex", 0x2AC },
{ "hstroke", 0x2B1 },
{ "hcircumflex", 0x2B6 },
{ "idotless", 0x2B9 },
{ "gbreve", 0x2BB },
{ "jcircumflex", 0x2BC },
{ "Cabovedot", 0x2C5 },
{ "Ccircumflex", 0x2C6 },
{ "Gabovedot", 0x2D5 },
{ "Gcircumflex", 0x2D8 },
{ "Ubreve", 0x2DD },
{ "Scircumflex", 0x2DE },
{ "cabovedot", 0x2E5 },
{ "ccircumflex", 0x2E6 },
{ "gabovedot", 0x2F5 },
{ "gcircumflex", 0x2F8 },
{ "ubreve", 0x2FD },
{ "scircumflex", 0x2FE },
{ "kra", 0x3A2 },
{ "kappa", 0x3A2 },
{ "Rcedilla", 0x3A3 },
{ "Itilde", 0x3A5 },
{ "Lcedilla", 0x3A6 },
{ "Emacron", 0x3AA },
{ "Gcedilla", 0x3AB },
{ "Tslash", 0x3AC },
{ "rcedilla", 0x3B3 },
{ "itilde", 0x3B5 },
{ "lcedilla", 0x3B6 },
{ "emacron", 0x3BA },
{ "gcedilla", 0x3BB },
{ "gacute", 0x3BB },
{ "tslash", 0x3BC },
{ "ENG", 0x3BD },
{ "eng", 0x3BF },
{ "Amacron", 0x3C0 },
{ "Iogonek", 0x3C7 },
{ "Eabovedot", 0x3CC },
{ "Imacron", 0x3CF },
{ "Ncedilla", 0x3D1 },
{ "Omacron", 0x3D2 },
{ "Kcedilla", 0x3D3 },
{ "Uogonek", 0x3D9 },
{ "Utilde", 0x3DD },
{ "Umacron", 0x3DE },
{ "amacron", 0x3E0 },
{ "iogonek", 0x3E7 },
{ "eabovedot", 0x3EC },
{ "imacron", 0x3EF },
{ "ncedilla", 0x3F1 },
{ "omacron", 0x3F2 },
{ "kcedilla", 0x3F3 },
{ "uogonek", 0x3F9 },
{ "utilde", 0x3FD },
{ "umacron", 0x3FE },
{ "overline", 0x47E },
{ "kana_fullstop", 0x4A1 },
{ "kana_openingbracket", 0x4A2 },
{ "kana_closingbracket", 0x4A3 },
{ "kana_comma", 0x4A4 },
{ "kana_conjunctive", 0x4A5 },
{ "kana_middledot", 0x4A5 },
{ "kana_WO", 0x4A6 },
{ "kana_a", 0x4A7 },
{ "kana_i", 0x4A8 },
{ "kana_u", 0x4A9 },
{ "kana_e", 0x4AA },
{ "kana_o", 0x4AB },
{ "kana_ya", 0x4AC },
{ "kana_yu", 0x4AD },
{ "kana_yo", 0x4AE },
{ "kana_tsu", 0x4AF },
{ "kana_tu", 0x4AF },
{ "prolongedsound", 0x4B0 },
{ "kana_A", 0x4B1 },
{ "kana_I", 0x4B2 },
{ "kana_U", 0x4B3 },
{ "kana_E", 0x4B4 },
{ "kana_O", 0x4B5 },
{ "kana_KA", 0x4B6 },
{ "kana_KI", 0x4B7 },
{ "kana_KU", 0x4B8 },
{ "kana_KE", 0x4B9 },
{ "kana_KO", 0x4BA },
{ "kana_SA", 0x4BB },
{ "kana_SHI", 0x4BC },
{ "kana_SU", 0x4BD },
{ "kana_SE", 0x4BE },
{ "kana_SO", 0x4BF },
{ "kana_TA", 0x4C0 },
{ "kana_CHI", 0x4C1 },
{ "kana_TI", 0x4C1 },
{ "kana_TSU", 0x4C2 },
{ "kana_TU", 0x4C2 },
{ "kana_TE", 0x4C3 },
{ "kana_TO", 0x4C4 },
{ "kana_NA", 0x4C5 },
{ "kana_NI", 0x4C6 },
{ "kana_NU", 0x4C7 },
{ "kana_NE", 0x4C8 },
{ "kana_NO", 0x4C9 },
{ "kana_HA", 0x4CA },
{ "kana_HI", 0x4CB },
{ "kana_FU", 0x4CC },
{ "kana_HU", 0x4CC },
{ "kana_HE", 0x4CD },
{ "kana_HO", 0x4CE },
{ "kana_MA", 0x4CF },
{ "kana_MI", 0x4D0 },
{ "kana_MU", 0x4D1 },
{ "kana_ME", 0x4D2 },
{ "kana_MO", 0x4D3 },
{ "kana_YA", 0x4D4 },
{ "kana_YU", 0x4D5 },
{ "kana_YO", 0x4D6 },
{ "kana_RA", 0x4D7 },
{ "kana_RI", 0x4D8 },
{ "kana_RU", 0x4D9 },
{ "kana_RE", 0x4DA },
{ "kana_RO", 0x4DB },
{ "kana_WA", 0x4DC },
{ "kana_N", 0x4DD },
{ "voicedsound", 0x4DE },
{ "semivoicedsound", 0x4DF },
{ "kana_switch", 0xFF7E },
{ "Arabic_comma", 0x5AC },
{ "Arabic_semicolon", 0x5BB },
{ "Arabic_question_mark", 0x5BF },
{ "Arabic_hamza", 0x5C1 },
{ "Arabic_maddaonalef", 0x5C2 },
{ "Arabic_hamzaonalef", 0x5C3 },
{ "Arabic_hamzaonwaw", 0x5C4 },
{ "Arabic_hamzaunderalef", 0x5C5 },
{ "Arabic_hamzaonyeh", 0x5C6 },
{ "Arabic_alef", 0x5C7 },
{ "Arabic_beh", 0x5C8 },
{ "Arabic_tehmarbuta", 0x5C9 },
{ "Arabic_teh", 0x5CA },
{ "Arabic_theh", 0x5CB },
{ "Arabic_jeem", 0x5CC },
{ "Arabic_hah", 0x5CD },
{ "Arabic_khah", 0x5CE },
{ "Arabic_dal", 0x5CF },
{ "Arabic_thal", 0x5D0 },
{ "Arabic_ra", 0x5D1 },
{ "Arabic_zain", 0x5D2 },
{ "Arabic_seen", 0x5D3 },
{ "Arabic_sheen", 0x5D4 },
{ "Arabic_sad", 0x5D5 },
{ "Arabic_dad", 0x5D6 },
{ "Arabic_tah", 0x5D7 },
{ "Arabic_zah", 0x5D8 },
{ "Arabic_ain", 0x5D9 },
{ "Arabic_ghain", 0x5DA },
{ "Arabic_tatweel", 0x5E0 },
{ "Arabic_feh", 0x5E1 },
{ "Arabic_qaf", 0x5E2 },
{ "Arabic_kaf", 0x5E3 },
{ "Arabic_lam", 0x5E4 },
{ "Arabic_meem", 0x5E5 },
{ "Arabic_noon", 0x5E6 },
{ "Arabic_ha", 0x5E7 },
{ "Arabic_heh", 0x5E7 },
{ "Arabic_waw", 0x5E8 },
{ "Arabic_alefmaksura", 0x5E9 },
{ "Arabic_yeh", 0x5EA },
{ "Arabic_fathatan", 0x5EB },
{ "Arabic_dammatan", 0x5EC },
{ "Arabic_kasratan", 0x5ED },
{ "Arabic_fatha", 0x5EE },
{ "Arabic_damma", 0x5EF },
{ "Arabic_kasra", 0x5F0 },
{ "Arabic_shadda", 0x5F1 },
{ "Arabic_sukun", 0x5F2 },
{ "Arabic_switch", 0xFF7E },
{ "Serbian_dje", 0x6A1 },
{ "Macedonia_gje", 0x6A2 },
{ "Cyrillic_io", 0x6A3 },
{ "Ukrainian_ie", 0x6A4 },
{ "Ukranian_je", 0x6A4 },
{ "Macedonia_dse", 0x6A5 },
{ "Ukrainian_i", 0x6A6 },
{ "Ukranian_i", 0x6A6 },
{ "Ukrainian_yi", 0x6A7 },
{ "Ukranian_yi", 0x6A7 },
{ "Cyrillic_je", 0x6A8 },
{ "Serbian_je", 0x6A8 },
{ "Cyrillic_lje", 0x6A9 },
{ "Serbian_lje", 0x6A9 },
{ "Cyrillic_nje", 0x6AA },
{ "Serbian_nje", 0x6AA },
{ "Serbian_tshe", 0x6AB },
{ "Macedonia_kje", 0x6AC },
{ "Ukrainian_ghe_with_upturn", 0x6AD },
{ "Byelorussian_shortu", 0x6AE },
{ "Cyrillic_dzhe", 0x6AF },
{ "Serbian_dze", 0x6AF },
{ "numerosign", 0x6B0 },
{ "Serbian_DJE", 0x6B1 },
{ "Macedonia_GJE", 0x6B2 },
{ "Cyrillic_IO", 0x6B3 },
{ "Ukrainian_IE", 0x6B4 },
{ "Ukranian_JE", 0x6B4 },
{ "Macedonia_DSE", 0x6B5 },
{ "Ukrainian_I", 0x6B6 },
{ "Ukranian_I", 0x6B6 },
{ "Ukrainian_YI", 0x6B7 },
{ "Ukranian_YI", 0x6B7 },
{ "Cyrillic_JE", 0x6B8 },
{ "Serbian_JE", 0x6B8 },
{ "Cyrillic_LJE", 0x6B9 },
{ "Serbian_LJE", 0x6B9 },
{ "Cyrillic_NJE", 0x6BA },
{ "Serbian_NJE", 0x6BA },
{ "Serbian_TSHE", 0x6BB },
{ "Macedonia_KJE", 0x6BC },
{ "Ukrainian_GHE_WITH_UPTURN", 0x6BD },
{ "Byelorussian_SHORTU", 0x6BE },
{ "Cyrillic_DZHE", 0x6BF },
{ "Serbian_DZE", 0x6BF },
{ "Cyrillic_yu", 0x6C0 },
{ "Cyrillic_a", 0x6C1 },
{ "Cyrillic_be", 0x6C2 },
{ "Cyrillic_tse", 0x6C3 },
{ "Cyrillic_de", 0x6C4 },
{ "Cyrillic_ie", 0x6C5 },
{ "Cyrillic_ef", 0x6C6 },
{ "Cyrillic_ghe", 0x6C7 },
{ "Cyrillic_ha", 0x6C8 },
{ "Cyrillic_i", 0x6C9 },
{ "Cyrillic_shorti", 0x6CA },
{ "Cyrillic_ka", 0x6CB },
{ "Cyrillic_el", 0x6CC },
{ "Cyrillic_em", 0x6CD },
{ "Cyrillic_en", 0x6CE },
{ "Cyrillic_o", 0x6CF },
{ "Cyrillic_pe", 0x6D0 },
{ "Cyrillic_ya", 0x6D1 },
{ "Cyrillic_er", 0x6D2 },
{ "Cyrillic_es", 0x6D3 },
{ "Cyrillic_te", 0x6D4 },
{ "Cyrillic_u", 0x6D5 },
{ "Cyrillic_zhe", 0x6D6 },
{ "Cyrillic_ve", 0x6D7 },
{ "Cyrillic_softsign", 0x6D8 },
{ "Cyrillic_yeru", 0x6D9 },
{ "Cyrillic_ze", 0x6DA },
{ "Cyrillic_sha", 0x6DB },
{ "Cyrillic_e", 0x6DC },
{ "Cyrillic_shcha", 0x6DD },
{ "Cyrillic_che", 0x6DE },
{ "Cyrillic_hardsign", 0x6DF },
{ "Cyrillic_YU", 0x6E0 },
{ "Cyrillic_A", 0x6E1 },
{ "Cyrillic_BE", 0x6E2 },
{ "Cyrillic_TSE", 0x6E3 },
{ "Cyrillic_DE", 0x6E4 },
{ "Cyrillic_IE", 0x6E5 },
{ "Cyrillic_EF", 0x6E6 },
{ "Cyrillic_GHE", 0x6E7 },
{ "Cyrillic_HA", 0x6E8 },
{ "Cyrillic_I", 0x6E9 },
{ "Cyrillic_SHORTI", 0x6EA },
{ "Cyrillic_KA", 0x6EB },
{ "Cyrillic_EL", 0x6EC },
{ "Cyrillic_EM", 0x6ED },
{ "Cyrillic_EN", 0x6EE },
{ "Cyrillic_O", 0x6EF },
{ "Cyrillic_PE", 0x6F0 },
{ "Cyrillic_YA", 0x6F1 },
{ "Cyrillic_ER", 0x6F2 },
{ "Cyrillic_ES", 0x6F3 },
{ "Cyrillic_TE", 0x6F4 },
{ "Cyrillic_U", 0x6F5 },
{ "Cyrillic_ZHE", 0x6F6 },
{ "Cyrillic_VE", 0x6F7 },
{ "Cyrillic_SOFTSIGN", 0x6F8 },
{ "Cyrillic_YERU", 0x6F9 },
{ "Cyrillic_ZE", 0x6FA },
{ "Cyrillic_SHA", 0x6FB },
{ "Cyrillic_E", 0x6FC },
{ "Cyrillic_SHCHA", 0x6FD },
{ "Cyrillic_CHE", 0x6FE },
{ "Cyrillic_HARDSIGN", 0x6FF },
{ "Greek_ALPHAaccent", 0x7A1 },
{ "Greek_EPSILONaccent", 0x7A2 },
{ "Greek_ETAaccent", 0x7A3 },
{ "Greek_IOTAaccent", 0x7A4 },
{ "Greek_IOTAdieresis", 0x7A5 },
{ "Greek_IOTAdiaeresis", 0x7A5 },
{ "Greek_IOTAaccentdiaeresis", 0x7A6 },
{ "Greek_OMICRONaccent", 0x7A7 },
{ "Greek_UPSILONaccent", 0x7A8 },
{ "Greek_UPSILONdieresis", 0x7A9 },
{ "Greek_UPSILONaccentdieresis", 0x7AA },
{ "Greek_OMEGAaccent", 0x7AB },
{ "Greek_accentdieresis", 0x7AE },
{ "Greek_horizbar", 0x7AF },
{ "Greek_alphaaccent", 0x7B1 },
{ "Greek_epsilonaccent", 0x7B2 },
{ "Greek_etaaccent", 0x7B3 },
{ "Greek_iotaaccent", 0x7B4 },
{ "Greek_iotadieresis", 0x7B5 },
{ "Greek_iotaaccentdieresis", 0x7B6 },
{ "Greek_omicronaccent", 0x7B7 },
{ "Greek_upsilonaccent", 0x7B8 },
{ "Greek_upsilondieresis", 0x7B9 },
{ "Greek_upsilonaccentdieresis", 0x7BA },
{ "Greek_omegaaccent", 0x7BB },
{ "Greek_ALPHA", 0x7C1 },
{ "Greek_BETA", 0x7C2 },
{ "Greek_GAMMA", 0x7C3 },
{ "Greek_DELTA", 0x7C4 },
{ "Greek_EPSILON", 0x7C5 },
{ "Greek_ZETA", 0x7C6 },
{ "Greek_ETA", 0x7C7 },
{ "Greek_THETA", 0x7C8 },
{ "Greek_IOTA", 0x7C9 },
{ "Greek_KAPPA", 0x7CA },
{ "Greek_LAMDA", 0x7CB },
{ "Greek_LAMBDA", 0x7CB },
{ "Greek_MU", 0x7CC },
{ "Greek_NU", 0x7CD },
{ "Greek_XI", 0x7CE },
{ "Greek_OMICRON", 0x7CF },
{ "Greek_PI", 0x7D0 },
{ "Greek_RHO", 0x7D1 },
{ "Greek_SIGMA", 0x7D2 },
{ "Greek_TAU", 0x7D4 },
{ "Greek_UPSILON", 0x7D5 },
{ "Greek_PHI", 0x7D6 },
{ "Greek_CHI", 0x7D7 },
{ "Greek_PSI", 0x7D8 },
{ "Greek_OMEGA", 0x7D9 },
{ "Greek_alpha", 0x7E1 },
{ "Greek_beta", 0x7E2 },
{ "Greek_gamma", 0x7E3 },
{ "Greek_delta", 0x7E4 },
{ "Greek_epsilon", 0x7E5 },
{ "Greek_zeta", 0x7E6 },
{ "Greek_eta", 0x7E7 },
{ "Greek_theta", 0x7E8 },
{ "Greek_iota", 0x7E9 },
{ "Greek_kappa", 0x7EA },
{ "Greek_lamda", 0x7EB },
{ "Greek_lambda", 0x7EB },
{ "Greek_mu", 0x7EC },
{ "Greek_nu", 0x7ED },
{ "Greek_xi", 0x7EE },
{ "Greek_omicron", 0x7EF },
{ "Greek_pi", 0x7F0 },
{ "Greek_rho", 0x7F1 },
{ "Greek_sigma", 0x7F2 },
{ "Greek_finalsmallsigma", 0x7F3 },
{ "Greek_tau", 0x7F4 },
{ "Greek_upsilon", 0x7F5 },
{ "Greek_phi", 0x7F6 },
{ "Greek_chi", 0x7F7 },
{ "Greek_psi", 0x7F8 },
{ "Greek_omega", 0x7F9 },
{ "Greek_switch", 0xFF7E },
{ "leftradical", 0x8A1 },
{ "topleftradical", 0x8A2 },
{ "horizconnector", 0x8A3 },
{ "topintegral", 0x8A4 },
{ "botintegral", 0x8A5 },
{ "vertconnector", 0x8A6 },
{ "topleftsqbracket", 0x8A7 },
{ "botleftsqbracket", 0x8A8 },
{ "toprightsqbracket", 0x8A9 },
{ "botrightsqbracket", 0x8AA },
{ "topleftparens", 0x8AB },
{ "botleftparens", 0x8AC },
{ "toprightparens", 0x8AD },
{ "botrightparens", 0x8AE },
{ "leftmiddlecurlybrace", 0x8AF },
{ "rightmiddlecurlybrace", 0x8B0 },
{ "topleftsummation", 0x8B1 },
{ "botleftsummation", 0x8B2 },
{ "topvertsummationconnector", 0x8B3 },
{ "botvertsummationconnector", 0x8B4 },
{ "toprightsummation", 0x8B5 },
{ "botrightsummation", 0x8B6 },
{ "rightmiddlesummation", 0x8B7 },
{ "lessthanequal", 0x8BC },
{ "notequal", 0x8BD },
{ "greaterthanequal", 0x8BE },
{ "integral", 0x8BF },
{ "therefore", 0x8C0 },
{ "variation", 0x8C1 },
{ "infinity", 0x8C2 },
{ "nabla", 0x8C5 },
{ "approximate", 0x8C8 },
{ "similarequal", 0x8C9 },
{ "ifonlyif", 0x8CD },
{ "implies", 0x8CE },
{ "identical", 0x8CF },
{ "radical", 0x8D6 },
{ "includedin", 0x8DA },
{ "includes", 0x8DB },
{ "intersection", 0x8DC },
{ "union", 0x8DD },
{ "logicaland", 0x8DE },
{ "logicalor", 0x8DF },
{ "partialderivative", 0x8EF },
{ "function", 0x8F6 },
{ "leftarrow", 0x8FB },
{ "uparrow", 0x8FC },
{ "rightarrow", 0x8FD },
{ "downarrow", 0x8FE },
{ "blank", 0x9DF },
{ "soliddiamond", 0x9E0 },
{ "checkerboard", 0x9E1 },
{ "ht", 0x9E2 },
{ "ff", 0x9E3 },
{ "cr", 0x9E4 },
{ "lf", 0x9E5 },
{ "nl", 0x9E8 },
{ "vt", 0x9E9 },
{ "lowrightcorner", 0x9EA },
{ "uprightcorner", 0x9EB },
{ "upleftcorner", 0x9EC },
{ "lowleftcorner", 0x9ED },
{ "crossinglines", 0x9EE },
{ "horizlinescan1", 0x9EF },
{ "horizlinescan3", 0x9F0 },
{ "horizlinescan5", 0x9F1 },
{ "horizlinescan7", 0x9F2 },
{ "horizlinescan9", 0x9F3 },
{ "leftt", 0x9F4 },
{ "rightt", 0x9F5 },
{ "bott", 0x9F6 },
{ "topt", 0x9F7 },
{ "vertbar", 0x9F8 },
{ "emspace", 0xAA1 },
{ "enspace", 0xAA2 },
{ "em3space", 0xAA3 },
{ "em4space", 0xAA4 },
{ "digitspace", 0xAA5 },
{ "punctspace", 0xAA6 },
{ "thinspace", 0xAA7 },
{ "hairspace", 0xAA8 },
{ "emdash", 0xAA9 },
{ "endash", 0xAAA },
{ "signifblank", 0xAAC },
{ "ellipsis", 0xAAE },
{ "doubbaselinedot", 0xAAF },
{ "onethird", 0xAB0 },
{ "twothirds", 0xAB1 },
{ "onefifth", 0xAB2 },
{ "twofifths", 0xAB3 },
{ "threefifths", 0xAB4 },
{ "fourfifths", 0xAB5 },
{ "onesixth", 0xAB6 },
{ "fivesixths", 0xAB7 },
{ "careof", 0xAB8 },
{ "figdash", 0xABB },
{ "leftanglebracket", 0xABC },
{ "decimalpoint", 0xABD },
{ "rightanglebracket", 0xABE },
{ "marker", 0xABF },
{ "oneeighth", 0xAC3 },
{ "threeeighths", 0xAC4 },
{ "fiveeighths", 0xAC5 },
{ "seveneighths", 0xAC6 },
{ "trademark", 0xAC9 },
{ "signaturemark", 0xACA },
{ "trademarkincircle", 0xACB },
{ "leftopentriangle", 0xACC },
{ "rightopentriangle", 0xACD },
{ "emopencircle", 0xACE },
{ "emopenrectangle", 0xACF },
{ "leftsinglequotemark", 0xAD0 },
{ "rightsinglequotemark", 0xAD1 },
{ "leftdoublequotemark", 0xAD2 },
{ "rightdoublequotemark", 0xAD3 },
{ "prescription", 0xAD4 },
{ "permille", 0xAD5 },
{ "minutes", 0xAD6 },
{ "seconds", 0xAD7 },
{ "latincross", 0xAD9 },
{ "hexagram", 0xADA },
{ "filledrectbullet", 0xADB },
{ "filledlefttribullet", 0xADC },
{ "filledrighttribullet", 0xADD },
{ "emfilledcircle", 0xADE },
{ "emfilledrect", 0xADF },
{ "enopencircbullet", 0xAE0 },
{ "enopensquarebullet", 0xAE1 },
{ "openrectbullet", 0xAE2 },
{ "opentribulletup", 0xAE3 },
{ "opentribulletdown", 0xAE4 },
{ "openstar", 0xAE5 },
{ "enfilledcircbullet", 0xAE6 },
{ "enfilledsqbullet", 0xAE7 },
{ "filledtribulletup", 0xAE8 },
{ "filledtribulletdown", 0xAE9 },
{ "leftpointer", 0xAEA },
{ "rightpointer", 0xAEB },
{ "club", 0xAEC },
{ "diamond", 0xAED },
{ "heart", 0xAEE },
{ "maltesecross", 0xAF0 },
{ "dagger", 0xAF1 },
{ "doubledagger", 0xAF2 },
{ "checkmark", 0xAF3 },
{ "ballotcross", 0xAF4 },
{ "musicalsharp", 0xAF5 },
{ "musicalflat", 0xAF6 },
{ "malesymbol", 0xAF7 },
{ "femalesymbol", 0xAF8 },
{ "telephone", 0xAF9 },
{ "telephonerecorder", 0xAFA },
{ "phonographcopyright", 0xAFB },
{ "caret", 0xAFC },
{ "singlelowquotemark", 0xAFD },
{ "doublelowquotemark", 0xAFE },
{ "cursor", 0xAFF },
{ "leftcaret", 0xBA3 },
{ "rightcaret", 0xBA6 },
{ "downcaret", 0xBA8 },
{ "upcaret", 0xBA9 },
{ "overbar", 0xBC0 },
{ "downtack", 0xBC2 },
{ "upshoe", 0xBC3 },
{ "downstile", 0xBC4 },
{ "underbar", 0xBC6 },
{ "jot", 0xBCA },
{ "quad", 0xBCC },
{ "uptack", 0xBCE },
{ "circle", 0xBCF },
{ "upstile", 0xBD3 },
{ "downshoe", 0xBD6 },
{ "rightshoe", 0xBD8 },
{ "leftshoe", 0xBDA },
{ "lefttack", 0xBDC },
{ "righttack", 0xBFC },
{ "hebrew_doublelowline", 0xCDF },
{ "hebrew_aleph", 0xCE0 },
{ "hebrew_bet", 0xCE1 },
{ "hebrew_beth", 0xCE1 },
{ "hebrew_gimel", 0xCE2 },
{ "hebrew_gimmel", 0xCE2 },
{ "hebrew_dalet", 0xCE3 },
{ "hebrew_daleth", 0xCE3 },
{ "hebrew_he", 0xCE4 },
{ "hebrew_waw", 0xCE5 },
{ "hebrew_zain", 0xCE6 },
{ "hebrew_zayin", 0xCE6 },
{ "hebrew_chet", 0xCE7 },
{ "hebrew_het", 0xCE7 },
{ "hebrew_tet", 0xCE8 },
{ "hebrew_teth", 0xCE8 },
{ "hebrew_yod", 0xCE9 },
{ "hebrew_finalkaph", 0xCEA },
{ "hebrew_kaph", 0xCEB },
{ "hebrew_lamed", 0xCEC },
{ "hebrew_finalmem", 0xCED },
{ "hebrew_mem", 0xCEE },
{ "hebrew_finalnun", 0xCEF },
{ "hebrew_nun", 0xCF0 },
{ "hebrew_samech", 0xCF1 },
{ "hebrew_samekh", 0xCF1 },
{ "hebrew_ayin", 0xCF2 },
{ "hebrew_finalpe", 0xCF3 },
{ "hebrew_pe", 0xCF4 },
{ "hebrew_finalzade", 0xCF5 },
{ "hebrew_finalzadi", 0xCF5 },
{ "hebrew_zade", 0xCF6 },
{ "hebrew_zadi", 0xCF6 },
{ "hebrew_qoph", 0xCF7 },
{ "hebrew_kuf", 0xCF7 },
{ "hebrew_resh", 0xCF8 },
{ "hebrew_shin", 0xCF9 },
{ "hebrew_taw", 0xCFA },
{ "hebrew_taf", 0xCFA },
{ "Hebrew_switch", 0xFF7E },
{ "XF86ModeLock", 0x1008FF01 },
{ "XF86MonBrightnessUp", 0x1008FF02 },
{ "XF86MonBrightnessDown", 0x1008FF03 },
{ "XF86KbdLightOnOff", 0x1008FF04 },
{ "XF86KbdBrightnessUp", 0x1008FF05 },
{ "XF86KbdBrightnessDown", 0x1008FF06 },
{ "XF86MonBrightnessCycle", 0x1008FF07 },
{ "XF86Standby", 0x1008FF10 },
{ "XF86AudioLowerVolume", 0x1008FF11 },
{ "XF86AudioMute", 0x1008FF12 },
{ "XF86AudioRaiseVolume", 0x1008FF13 },
{ "XF86AudioPlay", 0x1008FF14 },
{ "XF86AudioStop", 0x1008FF15 },
{ "XF86AudioPrev", 0x1008FF16 },
{ "XF86AudioNext", 0x1008FF17 },
{ "XF86HomePage", 0x1008FF18 },
{ "XF86Mail", 0x1008FF19 },
{ "XF86Start", 0x1008FF1A },
{ "XF86Search", 0x1008FF1B },
{ "XF86AudioRecord", 0x1008FF1C },
{ "XF86Calculator", 0x1008FF1D },
{ "XF86Memo", 0x1008FF1E },
{ "XF86ToDoList", 0x1008FF1F },
{ "XF86Calendar", 0x1008FF20 },
{ "XF86PowerDown", 0x1008FF21 },
{ "XF86ContrastAdjust", 0x1008FF22 },
{ "XF86RockerUp", 0x1008FF23 },
{ "XF86RockerDown", 0x1008FF24 },
{ "XF86RockerEnter", 0x1008FF25 },
{ "XF86Back", 0x1008FF26 },
{ "XF86Forward", 0x1008FF27 },
{ "XF86Stop", 0x1008FF28 },
{ "XF86Refresh", 0x1008FF29 },
{ "XF86PowerOff", 0x1008FF2A },
{ "XF86WakeUp", 0x1008FF2B },
{ "XF86Eject", 0x1008FF2C },
{ "XF86ScreenSaver", 0x1008FF2D },
{ "XF86WWW", 0x1008FF2E },
{ "XF86Sleep", 0x1008FF2F },
{ "XF86Favorites", 0x1008FF30 },
{ "XF86AudioPause", 0x1008FF31 },
{ "XF86AudioMedia", 0x1008FF32 },
{ "XF86MyComputer", 0x1008FF33 },
{ "XF86VendorHome", 0x1008FF34 },
{ "XF86LightBulb", 0x1008FF35 },
{ "XF86Shop", 0x1008FF36 },
{ "XF86History", 0x1008FF37 },
{ "XF86OpenURL", 0x1008FF38 },
{ "XF86AddFavorite", 0x1008FF39 },
{ "XF86HotLinks", 0x1008FF3A },
{ "XF86BrightnessAdjust", 0x1008FF3B },
{ "XF86Finance", 0x1008FF3C },
{ "XF86Community", 0x1008FF3D },
{ "XF86AudioRewind", 0x1008FF3E },
{ "XF86BackForward", 0x1008FF3F },
{ "XF86Launch0", 0x1008FF40 },
{ "XF86Launch1", 0x1008FF41 },
{ "XF86Launch2", 0x1008FF42 },
{ "XF86Launch3", 0x1008FF43 },
{ "XF86Launch4", 0x1008FF44 },
{ "XF86Launch5", 0x1008FF45 },
{ "XF86Launch6", 0x1008FF46 },
{ "XF86Launch7", 0x1008FF47 },
{ "XF86Launch8", 0x1008FF48 },
{ "XF86Launch9", 0x1008FF49 },
{ "XF86LaunchA", 0x1008FF4A },
{ "XF86LaunchB", 0x1008FF4B },
{ "XF86LaunchC", 0x1008FF4C },
{ "XF86LaunchD", 0x1008FF4D },
{ "XF86LaunchE", 0x1008FF4E },
{ "XF86LaunchF", 0x1008FF4F },
{ "XF86ApplicationLeft", 0x1008FF50 },
{ "XF86ApplicationRight", 0x1008FF51 },
{ "XF86Book", 0x1008FF52 },
{ "XF86CD", 0x1008FF53 },
{ "XF86Calculater", 0x1008FF54 },
{ "XF86Clear", 0x1008FF55 },
{ "XF86Close", 0x1008FF56 },
{ "XF86Copy", 0x1008FF57 },
{ "XF86Cut", 0x1008FF58 },
{ "XF86Display", 0x1008FF59 },
{ "XF86DOS", 0x1008FF5A },
{ "XF86Documents", 0x1008FF5B },
{ "XF86Excel", 0x1008FF5C },
{ "XF86Explorer", 0x1008FF5D },
{ "XF86Game", 0x1008FF5E },
{ "XF86Go", 0x1008FF5F },
{ "XF86iTouch", 0x1008FF60 },
{ "XF86LogOff", 0x1008FF61 },
{ "XF86Market", 0x1008FF62 },
{ "XF86Meeting", 0x1008FF63 },
{ "XF86MenuKB", 0x1008FF65 },
{ "XF86MenuPB", 0x1008FF66 },
{ "XF86MySites", 0x1008FF67 },
{ "XF86New", 0x1008FF68 },
{ "XF86News", 0x1008FF69 },
{ "XF86OfficeHome", 0x1008FF6A },
{ "XF86Open", 0x1008FF6B },
{ "XF86Option", 0x1008FF6C },
{ "XF86Paste", 0x1008FF6D },
{ "XF86Phone", 0x1008FF6E },
{ "XF86Q", 0x1008FF70 },
{ "XF86Reply", 0x1008FF72 },
{ "XF86Reload", 0x1008FF73 },
{ "XF86RotateWindows", 0x1008FF74 },
{ "XF86RotationPB", 0x1008FF75 },
{ "XF86RotationKB", 0x1008FF76 },
{ "XF86Save", 0x1008FF77 },
{ "XF86ScrollUp", 0x1008FF78 },
{ "XF86ScrollDown", 0x1008FF79 },
{ "XF86ScrollClick", 0x1008FF7A },
{ "XF86Send", 0x1008FF7B },
{ "XF86Spell", 0x1008FF7C },
{ "XF86SplitScreen", 0x1008FF7D },
{ "XF86Support", 0x1008FF7E },
{ "XF86TaskPane", 0x1008FF7F },
{ "XF86Terminal", 0x1008FF80 },
{ "XF86Tools", 0x1008FF81 },
{ "XF86Travel", 0x1008FF82 },
{ "XF86UserPB", 0x1008FF84 },
{ "XF86User1KB", 0x1008FF85 },
{ "XF86User2KB", 0x1008FF86 },
{ "XF86Video", 0x1008FF87 },
{ "XF86WheelButton", 0x1008FF88 },
{ "XF86Word", 0x1008FF89 },
{ "XF86Xfer", 0x1008FF8A },
{ "XF86ZoomIn", 0x1008FF8B },
{ "XF86ZoomOut", 0x1008FF8C },
{ "XF86Away", 0x1008FF8D },
{ "XF86Messenger", 0x1008FF8E },
{ "XF86WebCam", 0x1008FF8F },
{ "XF86MailForward", 0x1008FF90 },
{ "XF86Pictures", 0x1008FF91 },
{ "XF86Music", 0x1008FF92 },
{ "XF86Battery", 0x1008FF93 },
{ "XF86Bluetooth", 0x1008FF94 },
{ "XF86WLAN", 0x1008FF95 },
{ "XF86UWB", 0x1008FF96 },
{ "XF86AudioForward", 0x1008FF97 },
{ "XF86AudioRepeat", 0x1008FF98 },
{ "XF86AudioRandomPlay", 0x1008FF99 },
{ "XF86Subtitle", 0x1008FF9A },
{ "XF86AudioCycleTrack", 0x1008FF9B },
{ "XF86CycleAngle", 0x1008FF9C },
{ "XF86FrameBack", 0x1008FF9D },
{ "XF86FrameForward", 0x1008FF9E },
{ "XF86Time", 0x1008FF9F },
{ "XF86Select", 0x1008FFA0 },
{ "XF86View", 0x1008FFA1 },
{ "XF86TopMenu", 0x1008FFA2 },
{ "XF86Red", 0x1008FFA3 },
{ "XF86Green", 0x1008FFA4 },
{ "XF86Yellow", 0x1008FFA5 },
{ "XF86Blue", 0x1008FFA6 },
{ "XF86Suspend", 0x1008FFA7 },
{ "XF86Hibernate", 0x1008FFA8 },
{ "XF86TouchpadToggle", 0x1008FFA9 },
{ "XF86TouchpadOn", 0x1008FFB0 },
{ "XF86TouchpadOff", 0x1008FFB1 },
{ "XF86AudioMicMute", 0x1008FFB2 },
{ "XF86Keyboard", 0x1008FFB3 },
{ "XF86WWAN", 0x1008FFB4 },
{ "XF86RFKill", 0x1008FFB5 },
{ "XF86AudioPreset", 0x1008FFB6 },
{ "XF86RotationLockToggle", 0x1008FFB7 },
{ "XF86Switch_VT_1", 0x1008FE01 },
{ "XF86Switch_VT_2", 0x1008FE02 },
{ "XF86Switch_VT_3", 0x1008FE03 },
{ "XF86Switch_VT_4", 0x1008FE04 },
{ "XF86Switch_VT_5", 0x1008FE05 },
{ "XF86Switch_VT_6", 0x1008FE06 },
{ "XF86Switch_VT_7", 0x1008FE07 },
{ "XF86Switch_VT_8", 0x1008FE08 },
{ "XF86Switch_VT_9", 0x1008FE09 },
{ "XF86Switch_VT_10", 0x1008FE0A },
{ "XF86Switch_VT_11", 0x1008FE0B },
{ "XF86Switch_VT_12", 0x1008FE0C },
{ "XF86Ungrab", 0x1008FE20 },
{ "XF86ClearGrab", 0x1008FE21 },
{ "XF86Next_VMode", 0x1008FE22 },
{ "XF86Prev_VMode", 0x1008FE23 },
{ "XF86LogWindowTree", 0x1008FE24 },
{ "XF86LogGrabInfo", 0x1008FE25 },
