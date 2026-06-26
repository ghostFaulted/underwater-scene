// Definiujac ponizsze makro dokladnie jeden raz w calym projekcie (wlasnie w tym pliku .cpp),
// nakazujemy kompilatorowi nie tylko zadeklarowac naglowki z stb_image.h,
// ale wkleic takze fizycznie ciala funkcji biblioteki zeby linker mogl je pozniej polaczyc z reszta programu.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>