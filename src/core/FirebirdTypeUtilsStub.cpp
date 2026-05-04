#include <wx/wx.h>
#include "metadata/CharacterSet.h"
#include "metadata/database.h"

// Stubs for the dependencies of FirebirdTypeUtils
int CharacterSet::getBytesPerChar() const {
    return 1;
}

CharacterSetPtr Database::getCharsetById(int) {
    return CharacterSetPtr(); // This might crash if used, but it resolves the linker error
}
