// file iacamain.cc

// © 2016 Basile Starynkevitch
//   this file iacamain.cc is part of IaCa
//   IaCa is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   IaCa is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with IaCa.  If not, see <http://www.gnu.org/licenses/>.

#include "iaca.hh"

using namespace Iaca;

int main(int argc, char**argv)
{
  QApplication app(argc,argv);
  QApplication::setApplicationName("iaca");
  QApplication::setApplicationVersion(version);
  QCommandLineParser parser;
  parser.setApplicationDescription("the IaCa system descr");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.process(app);
  return app.exec();
}
