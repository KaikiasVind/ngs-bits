#include "AnalysisStatusWidget.h"
#include "MultiSampleDialog.h"
#include "TrioDialog.h"
#include "SomaticDialog.h"
#include "SingleSampleAnalysisDialog.h"
#include "GUIHelper.h"
#include "ScrollableTextDialog.h"
#include <QMenu>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QMetaMethod>

AnalysisStatusWidget::AnalysisStatusWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	//setup UI
	ui_.setupUi(this);
	GUIHelper::styleSplitter(ui_.splitter);
	connect(ui_.analyses, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_.analyses->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateDetails()));
	connect(ui_.refresh, SIGNAL(clicked(bool)), this, SLOT(refreshStatus()));
	ui_.f_date->setDate(QDate::currentDate().addDays(-14));
	connect(ui_.analysisSingle, SIGNAL(clicked(bool)), this, SLOT(analyzeSingleSamples()));
	connect(ui_.analysisTrio, SIGNAL(clicked(bool)), this, SLOT(analyzeTrio()));
	connect(ui_.analysisMulti, SIGNAL(clicked(bool)), this, SLOT(analyzeMultiSample()));
	connect(ui_.analysisSomatic, SIGNAL(clicked(bool)), this, SLOT(analyzeSomatic()));
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui_.f_text, SIGNAL(returnPressed()), this, SLOT(applyTextFilter()));
}

void AnalysisStatusWidget::analyzeSingleSamples(QList<AnalysisJobSample> samples)
{
	SingleSampleAnalysisDialog dlg(this);
	dlg.setSamples(samples);
	if (dlg.exec()==QDialog::Accepted)
	{
		NGSD db;
		foreach(const AnalysisJobSample& sample,  dlg.samples())
		{
			db.queueAnalysis("single sample", dlg.highPriority(), dlg.arguments(), QList<AnalysisJobSample>() << sample);
		}
		refreshStatus();
	}
}

void AnalysisStatusWidget::analyzeTrio(QList<AnalysisJobSample> samples)
{
	TrioDialog dlg(this);
	dlg.setSamples(samples);
	if (dlg.exec()==QDialog::Accepted)
	{
		NGSD().queueAnalysis("trio", dlg.highPriority(), dlg.arguments(), dlg.samples());
		refreshStatus();
	}
}

void AnalysisStatusWidget::analyzeMultiSample(QList<AnalysisJobSample> samples)
{
	MultiSampleDialog dlg(this);
	dlg.setSamples(samples);
	if (dlg.exec()==QDialog::Accepted)
	{
		NGSD().queueAnalysis("multi sample", dlg.highPriority(), dlg.arguments(), dlg.samples());
		refreshStatus();
	}
}

void AnalysisStatusWidget::analyzeSomatic(QList<AnalysisJobSample> samples)
{
	SomaticDialog dlg(this);
	dlg.setSamples(samples);
	if (dlg.exec()==QDialog::Accepted)
	{
		NGSD().queueAnalysis("somatic", dlg.highPriority(), dlg.arguments(), dlg.samples());
		refreshStatus();
	}
}

void AnalysisStatusWidget::refreshStatus()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//query job IDs
	NGSD db;
	SqlQuery query = db.getQuery();
	query.exec("SELECT id FROM analysis_job ORDER BY ID DESC");

	//clear contents
	ui_.analyses->clearContents();
	ui_.analyses->setRowCount(0);
	jobs_.clear();

	//add lines
	int row = -1;
	QSet<QString> tags_seen;
	while(query.next())
	{
		//get job infos
		int job_id = query.value(0).toInt();
		AnalysisJob job = db.analysisInfo(job_id);

		//check if job was already seen (i.e. it was repeated)
		QStringList tag_parts;
		tag_parts << job.type;
		foreach(const AnalysisJobSample& sample, job.samples)
		{
			tag_parts << sample.name + "|" + sample.info;
		}
		std::sort(tag_parts.begin(), tag_parts.end());
		QString tag = tag_parts.join(" ");
		bool repeated = tags_seen.contains(tag);
		tags_seen.insert(tag);

		//filter repeated
		if (repeated && !ui_.f_repeated->isChecked()) continue;

		//filter date
		QDateTime f_date = QDateTime(ui_.f_date->date());
		if (job.history.count()==0)
		{
			qDebug() << "No history for job " << job_id;
			continue;
		}

		if (job.history[0].time<f_date) break; //jobs are ordered by date => no newer jobs can come => skip the rest

		//get sample data
		QList<ProcessedSampleData> ps_data;
		foreach(const AnalysisJobSample& sample, job.samples)
		{
			ps_data << db.getProcessedSampleData(db.processedSampleId(sample.name));
		}

		//not filtered => add row
		++row;
		ui_.analyses->setRowCount(ui_.analyses->rowCount()+1);
		jobs_ << JobData{job_id, job, repeated};

		//queued
		if (job.history.count()>0)
		{
			addItem(ui_.analyses, row, 0, job.history[0].timeAsString());
			addItem(ui_.analyses, row, 1, job.history[0].user);
		}

		//type
		addItem(ui_.analyses, row, 2, job.type);

		//sample(s)
		QStringList parts;
		foreach(const AnalysisJobSample& sample, job.samples)
		{
			parts << sample.name;
		}
		addItem(ui_.analyses, row, 3, parts.join(" "));

		//system(s)
		parts.clear();
		foreach(const ProcessedSampleData& data, ps_data)
		{
			parts << data.processing_system;
		}
		parts.removeDuplicates();
		addItem(ui_.analyses, row, 4, parts.count()==1 ? parts[0] : "");

		//run(s)
		parts.clear();
		foreach(const ProcessedSampleData& data, ps_data)
		{
			parts << data.run_name;
		}
		parts.removeDuplicates();
		addItem(ui_.analyses, row, 5, parts.count()==1 ? parts[0] : "");

		//project(s)
		parts.clear();
		foreach(const ProcessedSampleData& data, ps_data)
		{
			parts << data.project_name;
		}
		parts.removeDuplicates();
		addItem(ui_.analyses, row, 6, parts.count()==1 ? parts[0] : "");

		//status
		QString status = job.finalStatus();
		QColor bg = statusToColor(status);
		if (job.sge_queue!="")
		{
			status += " (" + job.sge_queue + ")";
		}
		if (repeated)
		{
			bg = QColor("#D3D3D3");
			status += " > repeated";
		}

		addItem(ui_.analyses, row, 7, status, bg);
	}

	//apply text filter
	applyTextFilter();

	GUIHelper::resizeTableCells(ui_.analyses, 300);
	QApplication::restoreOverrideCursor();
}

void AnalysisStatusWidget::showContextMenu(QPoint pos)
{
	//extract selected rows
	QList<int> rows = selectedRows();

	//nothing selected => skip
	if (rows.isEmpty()) return;

	//extract processed sample data
	QList<AnalysisJobSample> samples;
	QSet<QString> types;
	QSet<int> job_ids;
	bool all_running = true;
	bool all_finished = true;
	foreach(int row, rows)
	{
		foreach(const AnalysisJobSample& sample, jobs_[row].job_data.samples)
		{
			if (!samples.contains(sample))
			{
				samples << sample;
			}
		}

		job_ids << jobs_[row].ngsd_id;
		types << jobs_[row].job_data.type;

		if (jobs_[row].job_data.isRunning())
		{
			all_finished = false;
		}
		else
		{
			all_running = false;
		}
	}

	//set up menu
	QMenu menu;
	menu.addAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample");
	if (rows.count()==1 && types.values()[0]!="single sample")
	{
		menu.addAction(QIcon(":/Icons/Folder.png"), "Open trio/multi/somatic folder");
	}
	menu.addAction(QIcon(":/Icons/Folder.png"), "Open sample folder(s)");
	if (all_running)
	{
		menu.addAction(QIcon(":/Icons/Remove.png"), "Cancel");
	}
	if (all_finished && types.count()==1)
	{
		QString type = types.values()[0];
		if (type=="single sample")
		{
			menu.addAction(QIcon(":/Icons/Refresh.png"), "Restart single sample analysis");
		}
		else if (type=="multi sample" && job_ids.count()==1)
		{
			menu.addAction(QIcon(":/Icons/Refresh.png"), "Restart multi-sample analysis");
		}
		else if (type=="trio" && job_ids.count()==1)
		{
			menu.addAction(QIcon(":/Icons/Refresh.png"), "Restart trio analysis");
		}
		else if (type=="somatic" && job_ids.count()==1)
		{
			menu.addAction(QIcon(":/Icons/Refresh.png"), "Restart somatic analysis");
		}
	}
	if (all_finished)
	{
		menu.addSeparator();
		menu.addAction(QIcon(":/Icons/Trash.png"), "Delete");
	}

	//show menu
	QAction* action = menu.exec(ui_.analyses->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	//execute
	QString text = action->text();
	if (text=="Open processed sample")
	{
		foreach(const AnalysisJobSample& sample, samples)
		{
			emit openProcessedSampleTab(sample.name);
		}
	}
	if (text=="Open sample folder(s)")
	{
		NGSD db;
		foreach(const AnalysisJobSample& sample, samples)
		{
			QDesktopServices::openUrl(db.processedSamplePath(db.processedSampleId(sample.name), NGSD::SAMPLE_FOLDER));
		}
	}
	if (text=="Open trio/multi/somatic folder")
	{
		NGSD db;
		QString folder = db.processedSamplePath(db.processedSampleId(samples[0].name), NGSD::PROJECT_FOLDER);

		//prefix
		QString type = types.values()[0];
		QString sample_sep = "_";
		if (type=="multi sample")
		{
			folder += "Multi_";
		}
		else if (type=="trio")
		{
			folder += "Trio_";
		}
		else if (type=="somatic")
		{
			folder += "Somatic_";
			sample_sep = "-";
		}
		else
		{
			THROW(ProgrammingException, "Unknown analysis type '" + type + "'!");
		}

		//samples
		bool first = true;
		foreach(const AnalysisJobSample& sample, samples)
		{
			if (!first)
			{
				folder += sample_sep;
			}
			folder += sample.name;
			first = false;
		}
		folder += "/";

		if (!QDesktopServices::openUrl(folder))
		{
			QMessageBox::warning(this, "Error opening folder", "Folder could not be opened - it probably does not exist (yet):\n" + folder);
		}
	}
	if (text=="Cancel")
	{
		foreach(int id, job_ids)
		{
			NGSD().cancelAnalysis(id);
		}
		refreshStatus();
	}
	if (text=="Restart single sample analysis")
	{
		analyzeSingleSamples(samples);
	}
	if (text=="Restart multi-sample analysis")
	{
		analyzeMultiSample(samples);
	}
	if (text=="Restart trio analysis")
	{
		analyzeTrio(samples);
	}
	if (text=="Restart somatic analysis")
	{
		analyzeSomatic(samples);
	}
	if (text=="Delete")
	{
		NGSD db;
		foreach(int id, job_ids)
		{
			db.deleteAnalysis(id);
		}
		refreshStatus();
	}
}

void AnalysisStatusWidget::clearDetails()
{
	//details
	ui_.properties->clearContents();
	ui_.properties->setRowCount(0);

	//samples
	ui_.samples->clearContents();
	ui_.samples->setRowCount(0);

	//history
	ui_.history->clearContents();
	ui_.history->setRowCount(0);
}

void AnalysisStatusWidget::updateDetails()
{
	clearDetails();

	//determine row
	QList<int> selected_rows = selectedRows();

	if (selected_rows.count()!=1) return;
	const AnalysisJob& job = jobs_[selected_rows[0]].job_data;

	//properties
	ui_.properties->setRowCount(5);
	addItem(ui_.properties, 0, 0, "high_priority");
	addItem(ui_.properties, 0, 1, job.high_priority ? "yes" : "no");
	addItem(ui_.properties, 1, 0, "arguments");
	addItem(ui_.properties, 1, 1, job.args);
	addItem(ui_.properties, 2, 0, "SGE id");
	addItem(ui_.properties, 2, 1, job.sge_id);
	addItem(ui_.properties, 3, 0, "SGE queue");
	addItem(ui_.properties, 3, 1, job.sge_queue);
	addItem(ui_.properties, 4, 0, "run time");
	addItem(ui_.properties, 4, 1, job.runTimeAsString());
	GUIHelper::resizeTableCells(ui_.properties);

	//samples
	ui_.samples->setRowCount(job.samples.count());
	int r = 0;
	foreach(const AnalysisJobSample& sample, job.samples)
	{
		addItem(ui_.samples, r, 0, sample.name);
		addItem(ui_.samples, r, 1, sample.info);
		++r;
	}
	GUIHelper::resizeTableCells(ui_.samples);

	//history
	ui_.history->setRowCount(job.history.count());
	r = 0;
	foreach(const AnalysisJobHistoryEntry& entry, job.history)
	{
		addItem(ui_.history, r, 0, entry.timeAsString());
		addItem(ui_.history, r, 1, entry.user);
		addItem(ui_.history, r, 2, entry.status, statusToColor(entry.status));
		QString output = entry.output.join("\n").trimmed();
		if (output.count()>50)
		{
			output.resize(50);
			output += "...";
		}
		addItem(ui_.history, r, 3, output);
		++r;
	}
	GUIHelper::resizeTableCells(ui_.history);

	//output
	ui_.output->setText(job.history.last().output.join("\n"));
}

void AnalysisStatusWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.analyses);
}

void AnalysisStatusWidget::applyTextFilter()
{
	QString f_text = ui_.f_text->text().trimmed();

	//no search string => show all
	if (f_text.isEmpty())
	{

		for (int r=0; r<ui_.analyses->rowCount(); ++r)
		{
			ui_.analyses->setRowHidden(r, false);
		}
		return;
	}

	//search
	for (int r=0; r<ui_.analyses->rowCount(); ++r)
	{
		bool match = false;
		for (int c=0; c<ui_.analyses->columnCount(); ++c)
		{
			QTableWidgetItem* item = ui_.analyses->item(r, c);
			if (item==nullptr) continue;

			if (item->text().contains(f_text, Qt::CaseInsensitive))
			{
				match = true;
				break;
			}
		}

		ui_.analyses->setRowHidden(r, !match);
	}
}

void AnalysisStatusWidget::addItem(QTableWidget* table, int row, int col, QString text, QColor bg_color)
{
	auto item = new QTableWidgetItem(text);
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	item->setBackgroundColor(bg_color);
	table->setItem(row, col, item);
}

QColor AnalysisStatusWidget::statusToColor(QString status)
{
	QColor output = Qt::transparent;
	if (status=="started") output = QColor("#90EE90");
	if (status=="finished") output = QColor("#44BB44");
	if (status=="cancel") output = QColor("#FFC45E");
	if (status=="canceled") output = QColor("#FFC45E");
	if (status=="error") output = QColor("#FF0000");
	return output;
}

QList<int> AnalysisStatusWidget::selectedRows() const
{
	QList<int> output;

	QList<QTableWidgetSelectionRange> ranges = ui_.analyses->selectedRanges();
	foreach(const QTableWidgetSelectionRange& range, ranges)
	{
		for (int r=range.topRow(); r<=range.bottomRow(); ++r)
		{
			if (ui_.analyses->isRowHidden(r)) continue;
			output << r;
		}
	}

	std::sort(output.begin(), output.end());

	return output;
}