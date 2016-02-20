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

bool Iaca::batch;

int main(int argc, char**argv)
{
    ItemPtr ip;
    batch = false;
    std::unique_ptr<QCoreApplication> this_app;
    for (int ix=1; ix<argc && !batch; ix++)
        if (!strcmp(argv[ix],"--batch") || !strcmp(argv[ix],"-B"))
            batch = true;
    if (batch)
        this_app.reset(new QCoreApplication(argc,argv));
    else
        this_app.reset(new QApplication(argc,argv));
    this_app->setApplicationName("iaca");
    this_app->setApplicationVersion(QString {iaca_lastgitcommit} .append(" (").append(iaca_timestamp).append(")"));
    {
        QCommandLineParser parser;
        parser.setApplicationDescription("the IaCa system descr");
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addOptions({
            {   {"b","batch"},
                QCoreApplication::translate("main","Run in batch mode without GUI.")
            }
        });
        parser.process(*this_app);
    }
    return this_app->exec();
}
