/*                   C A D I M P O R T . C X X
 * BRL-CAD
 *
 * Copyright (c) 2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file cadimport.cxx
 *
 * Logic for launching command line importers.  Eventually libgcv
 * will probably replace the explicit program running, but the option
 * dialogs will still be necessary in some form...
 *
 */

#include <QFileInfo>
#include <QFormLayout>

#include "cadapp.h"
#include "cadimport.h"

RhinoImportDialog::RhinoImportDialog(QString filename)
{
    input_file = filename;
    QFileInfo fileinfo(filename);
    QString g_path(fileinfo.path());
    g_path.append("/");
    g_path.append(fileinfo.baseName());
    QString l_path(g_path);
    g_path.append(".g");
    l_path.append(".log");

    db_path = new QLineEdit;
    db_path->insert(g_path);
    log_path = new QLineEdit;
    log_path->insert(l_path);
    scaling_factor = new QLineEdit;
    tolerance = new QLineEdit;
    verbosity = new QLineEdit;
    debug_printing = new QCheckBox;
    random_colors = new QCheckBox;
    uuid = new QCheckBox;

    formGroupBox = new QGroupBox("Import Options");
    QFormLayout *flayout = new QFormLayout;
    flayout->addRow(new QLabel("Output File"), db_path);
    flayout->addRow(new QLabel("Print Debugging Info"), debug_printing);
    flayout->addRow(new QLabel("Printing Verbosity"), verbosity);
    flayout->addRow(new QLabel("Scaling Factor"), scaling_factor);
    flayout->addRow(new QLabel("Tolerance"), tolerance);
    flayout->addRow(new QLabel("Randomize Colors"), random_colors);
    flayout->addRow(new QLabel("Use Universally Unique Identifiers\n(UUIDs) for Object Names"), uuid);
    flayout->addRow(new QLabel("Log File"), log_path);
    formGroupBox->setLayout(flayout);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
	    | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(formGroupBox);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setWindowTitle(tr("Rhino 3DM Importer (3dm-g)"));

}

QString
RhinoImportDialog::command_string()
{
    QString prog_name(bu_brlcad_dir("bin", 1));
    prog_name.append("/3dm-g");
    prog_name = QString(bu_brlcad_root((const char *)prog_name.toLocal8Bit(), 1));

    QString cmd(prog_name);

    if (debug_printing->isChecked()) cmd.append(" -d");
    if (verbosity->text().length() > 0) {
	cmd.append(" -v ");
	cmd.append(verbosity->text());
    }
    if (scaling_factor->text().length() > 0) {
	cmd.append(" -s ");
	cmd.append(scaling_factor->text());
    }
    if (tolerance->text().length() > 0) {
	cmd.append(" -t ");
	cmd.append(tolerance->text());
    }
    if (random_colors->isChecked()) cmd.append(" -r");
    if (uuid->isChecked()) cmd.append(" -u");

    // TODO - pop up a message and quit if we don't have an output file
    cmd.append(" -o ");
    cmd.append(db_path->text());

    cmd.append(" ");
    cmd.append(input_file);

    return cmd;
}

QString
import_db(QString filename) {

    QFileInfo fileinfo(filename);
    QString g_path("");
    QString conversion_command;

    if (!fileinfo.suffix().compare("g", Qt::CaseInsensitive)) return filename;

    /* If we're not a .g, time for the conversion */

    if (!fileinfo.suffix().compare("3dm", Qt::CaseInsensitive)) {
       RhinoImportDialog dialog(filename);
       dialog.exec();
       g_path = dialog.db_path->text();
       conversion_command = dialog.command_string();
       // TODO - integrate logging mechanism into command execution
       ((CADApp *)qApp)->exec_console_app_in_window(&conversion_command);
    }

    return g_path;
}

/*
 * Local Variables:
 * mode: C++
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */

