/*
 * Syscoin Developers 2015
 */
#include "myacceptedofferlistpage.h"
#include "ui_myacceptedofferlistpage.h"
#include "offeraccepttablemodel.h"
#include "offeracceptinfodialog.h"
#include "newmessagedialog.h"
#include "clientmodel.h"
#include "platformstyle.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "syscoingui.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "rpcserver.h"
using namespace std;

extern const CRPCTable tableRPC;

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
MyAcceptedOfferListPage::MyAcceptedOfferListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyAcceptedOfferListPage),
    model(0),
    optionsModel(0),
	platformStyle(platformStyle)
{
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->exportButton->setIcon(QIcon());
		ui->messageButton->setIcon(QIcon());
		ui->detailButton->setIcon(QIcon());
		ui->copyOffer->setIcon(QIcon());
		ui->refreshButton->setIcon(QIcon());

	}
	else
	{
		ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/export"));
		ui->messageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
		ui->detailButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/details"));
		ui->copyOffer->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/editcopy"));
		ui->refreshButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/refresh"));
		
	}

	ui->buttonBox->setVisible(false);

    ui->labelExplanation->setText(tr("These are offers you have sold to others. Offer operations take 2-5 minutes to become active. Right click on an offer for more info including buyer message, quantity, date, etc."));
	
	connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_detailButton_clicked()));	
    // Context menu actions
    QAction *copyOfferAction = new QAction(ui->copyOffer->text(), this);
    QAction *copyOfferValueAction = new QAction(tr("&Copy OfferAccept ID"), this);
	QAction *detailsAction = new QAction(tr("&Details"), this);
	QAction *messageAction = new QAction(tr("&Message Buyer"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyOfferAction);
    contextMenu->addAction(copyOfferValueAction);
	contextMenu->addSeparator();
	contextMenu->addAction(detailsAction);
	contextMenu->addAction(messageAction);
    // Connect signals for context menu actions
    connect(copyOfferAction, SIGNAL(triggered()), this, SLOT(on_copyOffer_clicked()));
    connect(copyOfferValueAction, SIGNAL(triggered()), this, SLOT(onCopyOfferValueAction()));
	connect(detailsAction, SIGNAL(triggered()), this, SLOT(on_detailButton_clicked()));
	connect(messageAction, SIGNAL(triggered()), this, SLOT(on_messageButton_clicked()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));


}
void MyAcceptedOfferListPage::on_messageButton_clicked()
{
 	if(!model)	
		return;
	if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(selection.isEmpty())
    {
        return;
    }
	QString buyer = selection.at(0).data(OfferAcceptTableModel::BuyerRole).toString();
	// send message to buyer
	NewMessageDialog dlg(NewMessageDialog::NewMessage, buyer);   
	dlg.exec();


}

MyAcceptedOfferListPage::~MyAcceptedOfferListPage()
{
    delete ui;
}
void MyAcceptedOfferListPage::on_detailButton_clicked()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        OfferAcceptInfoDialog dlg(platformStyle, selection.at(0));
        dlg.exec();
    }
}
void MyAcceptedOfferListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
    /*if(walletModel->getEncryptionStatus() == WalletModel::Locked)
	{
        ui->labelExplanation->setText(tr("<font color='blue'>WARNING: Your wallet is currently locked. For security purposes you'll need to enter your passphrase in order to interact with Syscoin Offers. Because your wallet is locked you must manually refresh this table after creating or updating an Offer. </font> <a href=\"http://lockedwallet.syscoin.org\">more info</a><br><br>These are your registered Syscoin Offers. Offer updates take 1 confirmation to appear in this table."));
		ui->labelExplanation->setTextFormat(Qt::RichText);
		ui->labelExplanation->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui->labelExplanation->setOpenExternalLinks(true);
    }*/
}
void MyAcceptedOfferListPage::setModel(WalletModel *walletModel, OfferAcceptTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

  
		// Receive filter
		proxyModel->setFilterRole(OfferAcceptTableModel::TypeRole);
		proxyModel->setFilterFixedString(OfferAcceptTableModel::Offer);
       
    
		ui->tableView->setModel(proxyModel);
		ui->tableView->sortByColumn(0, Qt::AscendingOrder);
        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);

        // Set column widths
        ui->tableView->setColumnWidth(0, 50); //offer
        ui->tableView->setColumnWidth(1, 50); //accept
        ui->tableView->setColumnWidth(2, 500); //title
        ui->tableView->setColumnWidth(3, 50); //height
        ui->tableView->setColumnWidth(4, 50); //price
        ui->tableView->setColumnWidth(5, 75); //currency
        ui->tableView->setColumnWidth(6, 75); //qty
        ui->tableView->setColumnWidth(7, 50); //total
        ui->tableView->setColumnWidth(8, 150); //seller alias
        ui->tableView->setColumnWidth(9, 150); //buyer
        ui->tableView->setColumnWidth(10, 40); //private
        ui->tableView->setColumnWidth(11, 0); //status

        ui->tableView->horizontalHeader()->setStretchLastSection(true);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    // Select row for newly created offer
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewOffer(QModelIndex,int,int)));

    selectionChanged();
}

void MyAcceptedOfferListPage::setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
	this->clientModel = clientmodel;
}

void MyAcceptedOfferListPage::on_copyOffer_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, OfferAcceptTableModel::Name);
}

void MyAcceptedOfferListPage::onCopyOfferValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, OfferAcceptTableModel::GUID);
}


void MyAcceptedOfferListPage::on_refreshButton_clicked()
{
    if(!model)
        return;
    model->refreshOfferTable();
}

void MyAcceptedOfferListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyOffer->setEnabled(true);
		ui->messageButton->setEnabled(true);
		ui->detailButton->setEnabled(true);
    }
    else
    {
        ui->copyOffer->setEnabled(false);
		ui->messageButton->setEnabled(false);
		ui->detailButton->setEnabled(false);
    }
}

void MyAcceptedOfferListPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which offer was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(OfferAcceptTableModel::Name);

    Q_FOREACH (const QModelIndex& index, indexes)
    {
        QVariant offer = table->model()->data(index);
        returnValue = offer.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no offer entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void MyAcceptedOfferListPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Offer Data"), QString(),
            tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Offer ID", OfferAcceptTableModel::Name, Qt::EditRole);
    writer.addColumn("OfferAccept ID", OfferAcceptTableModel::GUID, Qt::EditRole);
	writer.addColumn("Title", OfferAcceptTableModel::Title, Qt::EditRole);
	writer.addColumn("Height", OfferAcceptTableModel::Height, Qt::EditRole);
	writer.addColumn("Price", OfferAcceptTableModel::Price, Qt::EditRole);
	writer.addColumn("Currency", OfferAcceptTableModel::Currency, Qt::EditRole);
	writer.addColumn("Qty", OfferAcceptTableModel::Qty, Qt::EditRole);
	writer.addColumn("Total", OfferAcceptTableModel::Total, Qt::EditRole);
	writer.addColumn("Alias", OfferAcceptTableModel::Alias, Qt::EditRole);
	writer.addColumn("Status", OfferAcceptTableModel::Status, Qt::EditRole);
    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}



void MyAcceptedOfferListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void MyAcceptedOfferListPage::selectNewOffer(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, OfferAcceptTableModel::Name, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newOfferToSelect))
    {
        // Select row of newly created offer, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newOfferToSelect.clear();
    }
}