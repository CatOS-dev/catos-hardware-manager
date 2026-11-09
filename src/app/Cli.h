#pragma once

#include <QStringList>

// Headless modes for the installer and first-boot setup:
//   --list [--json]               devices, status and recommended changes
//   --apply-recommended [--yes]   apply what "Select recommended" would stage
namespace Cli {
bool wants(int argc, char **argv);
int run(int argc, char **argv);
}
