#include "TextModels.h"

namespace TextModels {

Attributes gDefAttrs = {&gFont, 0xff000000};


Rider::Rider(): fBase(NULL), fEra(0), fPos(0), fRun(NULL), fOfs(NULL)
{}

Rider::Rider(const Rider &src): fBase(src.fBase), fEra(src.fEra), fPos(src.fPos), fRun(src.fRun), fOfs(src.fOfs)
{
}


}
