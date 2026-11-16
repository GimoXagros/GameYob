#!/usr/bin/env python3
"""Exercise the production menu config functions without DS hardware headers."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
menu = (ROOT / 'platform/common/menu.cpp').read_text(encoding='utf8')
graphics = (ROOT / 'platform/common/include/gbgfx.h').read_text(encoding='utf8')


def function(signature):
    start = menu.index(signature)
    return menu[start:menu.index('\n}', start) + 2]


# Compile the actual parser and setter, rather than a second implementation.
enum_start = graphics.index('enum {')
scale_enum = graphics[enum_start:graphics.index('};', enum_start) + 2]
source = r'''
#include <cassert>
#include <cstring>
#include <cstdlib>
#ifdef _WIN32
#define strcasecmp _stricmp
#endif
static int calls;
static int callbackValue;
void changed(int value) { ++calls; callbackValue = value; }
struct Option {
    const char* name;
    int platforms;
    int numValues;
    int selection;
    void (*function)(int);
};
struct Menu { int numOptions; Option options[4]; };
static const int MENU_BITMASK = 1;
static const int numMenus = 1;
static Menu menuList[] = {{4, {
    {"Scaling", 1, 9, 0, changed},
    {"Vertical Pan", 1, 5, 2, changed},
    {"Save Settings", 1, 0, 0, changed},
    {"Unavailable", 2, 2, 0, changed}
}}};
static const char scalingConfigKey[] = "Scaling Mode";
'''
source += scale_enum + '\n'
source += function('void setMenuOption(') + '\n'
source += function('void menuParseConfig(') + '\n'
source += r'''
void parse(const char* input) {
    char line[80];
    std::strcpy(line, input);
    menuParseConfig(line);
}
int main() {
    for (const char* bad : {"Vertical Pan=-1", "Vertical Pan=5", "Vertical Pan=10"}) {
        int before = calls;
        parse(bad);
        assert(menuList[0].options[1].selection == 2 && calls == before);
    }
    parse("Vertical Pan=0");
    assert(menuList[0].options[1].selection == 0 && callbackValue == 0);
    parse("Vertical Pan=4");
    assert(menuList[0].options[1].selection == 4 && callbackValue == 4);
    int before = calls;
    setMenuOption("Vertical Pan", -1);
    setMenuOption("Vertical Pan", 5);
    parse("Save Settings=0");
    parse("Unavailable=1");
    assert(menuList[0].options[1].selection == 4 && calls == before);
    parse("Scaling=1");
    assert(menuList[0].options[0].selection == SCALE_ASPECT);
    parse("Scaling=2");
    assert(menuList[0].options[0].selection == SCALE_FULL);
    parse("Scaling=0");
    assert(menuList[0].options[0].selection == SCALE_OFF);
    parse("Scaling Mode=7");
    assert(menuList[0].options[0].selection == SCALE_1_6);
    before = calls;
    parse("Scaling=-1");
    parse("Scaling=3");
    parse("Scaling Mode=-1");
    parse("Scaling Mode=9");
    assert(menuList[0].options[0].selection == SCALE_1_6 && calls == before);
}
'''
source = '#include <initializer_list>\n' + source
with tempfile.TemporaryDirectory(prefix='gameyob-menu-config-') as directory:
    temporary = Path(directory)
    cpp = temporary / 'menu_config.cpp'
    binary = temporary / ('menu_config.exe' if os.name == 'nt' else 'menu_config')
    cpp.write_text(source, encoding='utf8')
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++11', '-Wall', '-Wextra',
                    '-Werror', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Production menu config range and legacy migration checks: PASS')
