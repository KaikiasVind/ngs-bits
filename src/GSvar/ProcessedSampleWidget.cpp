#include "ProcessedSampleWidget.h"
#include "ui_ProcessedSampleWidget.h"
#include "DBQCWidget.h"
#include "GUIHelper.h"

#include <QMessageBox>

ProcessedSampleWidget::ProcessedSampleWidget(QWidget* parent, QString ps_id)
	: QWidget(parent)
	, ui_(new Ui::ProcessedSampleWidget)
	, id_(ps_id)
{
	ui_->setupUi(this);

	QAction* action = new QAction("Plot", this);
	ui_->qc_table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(showPlot()));

	updateGUI();
}

ProcessedSampleWidget::~ProcessedSampleWidget()
{
	delete ui_;
}

void ProcessedSampleWidget::updateGUI()
{
	//#### processed sample details ####
	ProcessedSampleData ps_data = db_.getProcessedSampleData(id_);
	ui_->name->setText(ps_data.name);
	ui_->comment->setText(ps_data.comments);
	ui_->system->setText(ps_data.processing_system);
	ui_->project->setText(ps_data.project_name);
	QString quality = ps_data.quality;
	if (quality=="bad") quality = "<font color=red>bad</font>";
	ui_->quality->setText(quality);
	ui_->run->setText(ps_data.run_name);

	//#### sample details ####
	SampleData s_data = db_.getSampleData(db_.getValue("SELECT sample_id FROM processed_sample WHERE id='" + id_ + "'").toString());
	ui_->name_external->setText(s_data.name_external);
	ui_->gender->setText(s_data.gender);
	ui_->tumor_ffpe->setText(QString(s_data.is_tumor ? "<font color=red>yes</font>" : "no") + " / " + (s_data.is_ffpe ? "<font color=red>yes</font>" : "no"));
	ui_->disease_group->setText(s_data.disease_group);
	ui_->disease_status->setText(s_data.disease_status);

	//#### disease details ####
	DBTable dd_table = db_.createTable("sample_disease_info", "SELECT sdi.id, sdi.type, sdi.disease_info, u.name, sdi.date, t.name as hpo_name FROM user u, processed_sample ps, sample_disease_info sdi LEFT JOIN hpo_term t ON sdi.disease_info=t.hpo_id WHERE sdi.sample_id=ps.sample_id AND sdi.user_id=u.id AND ps.id='" + id_ + "' ORDER BY sdi.date ASC");
	//append merge HPO id and name
	QStringList hpo_names = dd_table.takeColumn(dd_table.columnIndex("hpo_name"));
	QStringList info_entries = dd_table.extractColumn(dd_table.columnIndex("disease_info"));
	for (int i=0; i<info_entries.count(); ++i)
	{
		if (info_entries[i].startsWith("HP:"))
		{
			info_entries[i] += " (" + hpo_names[i] + ")";
		}
	}
	dd_table.setColumn(dd_table.columnIndex("disease_info"), info_entries);
	ui_->disease_details->setData(dd_table);

	//#### QC ####
	DBTable qc_table = db_.createTable("processed_sample_qc", "SELECT qc.id, t.qcml_id, t.name, qc.value, t.description FROM processed_sample_qc qc, qc_terms t WHERE qc.qc_terms_id=t.id AND t.obsolete=0 AND qc.processed_sample_id='" + id_ + "' ORDER BY t.qcml_id ASC");
	//use descriptions as tooltip
	QStringList descriptions = qc_table.takeColumn(qc_table.columnIndex("description"));
	ui_->qc_table->setData(qc_table);
	ui_->qc_table->setColumnTooltips(qc_table.columnIndex("name"), descriptions);
}

void ProcessedSampleWidget::showPlot()
{
	QSet<int> selected_rows = ui_->qc_table->selectedRows();
	if (selected_rows.count()!=1)
	{
		QMessageBox::information(this, "Plot error", "Please select <b>one</b> quality metric for plotting!");
		return;
	}

	//get database IDs
	int r = *(selected_rows.begin());
	QString qc_term_id = db_.getValue("SELECT qc_terms_id FROM processed_sample_qc WHERE id='" + ui_->qc_table->getId(r) + "'").toString();
	QString sys_id = db_.getValue("SELECT ps.processing_system_id FROM processed_sample_qc qc, processed_sample ps WHERE qc.processed_sample_id=ps.id AND qc.id='" + ui_->qc_table->getId(r) + "'").toString();

	//show widget
	DBQCWidget* qc_widget = new DBQCWidget(this);
	qc_widget->setSystemId(sys_id);
	qc_widget->setTermId(qc_term_id);
	GUIHelper::showWidgetAsDialog(qc_widget, "QC plot", false);
}


//TODO: highlight QC:2000005, QC:2000007, QC:2000008, QC:2000009, QC:2000010, QC:2000013, QC:2000014, QC:2000015, QC:2000016, QC:2000017, QC:2000018, QC:2000019, QC:2000020, QC:2000021, QC:2000022, QC:2000023, QC:2000024, QC:2000025, QC:2000027, QC:2000049, QC:2000051
//TODO: zoom into charts and reset
//TODO: highlight current sample
