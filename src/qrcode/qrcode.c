#include <stdbool.h>
#include <stdint.h>
#include "qrcodegen.h"
#include "display.h"
#include "qrcode.h"
#include "qrcodegen.h"

static uint8_t qr0[qrcodegen_BUFFER_LEN_MAX];
static uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

#define MODULE_SIZE 1
// space around qr code
#define DEAD_SPACE  (4*MODULE_SIZE)

bool drawQrCode(char* text) {
	bool ok = qrcodegen_encodeText(text, tempBuffer, qr0, qrcodegen_Ecc_LOW,
	qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
	if (!ok)
		return false;

	//oledBox(0, 0, OLED_WIDTH, OLED_HEIGHT, true);
	int size = qrcodegen_getSize(qr0);
	int x0 = DEAD_SPACE + ((OLED_WIDTH - 2 * DEAD_SPACE) - MODULE_SIZE * size) / 2;
	int y0 = DEAD_SPACE + (((OLED_HEIGHT - 1 - FONT_HEIGHT - 1) - 2 * DEAD_SPACE) - MODULE_SIZE * size) / 2;
	oledBox(0, 0, OLED_WIDTH - 1, OLED_HEIGHT - 1, true);
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			if (qrcodegen_getModule(qr0, x, y)) {
				//oledClearPixel(x0 + x, y0 + y);
				oledBox(x0 + MODULE_SIZE * x, y0 + MODULE_SIZE * y, x0 + MODULE_SIZE * x + (MODULE_SIZE - 1),
						y0 + MODULE_SIZE * y + (MODULE_SIZE - 1),
						false);
			}
		}
	}
	return true;
}
