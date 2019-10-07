#ifndef __OTC_REQUEST_VIEW_MODEL_H__
#define __OTC_REQUEST_VIEW_MODEL_H__

#include <QAbstractTableModel>
#include <QTimer>

#include "OtcTypes.h"

class OtcClient;

enum class CustomRoles
{
   OwnQuote = Qt::UserRole + 1
};

class OTCRequestViewModel : public QAbstractTableModel
{
   Q_OBJECT

public:
   OTCRequestViewModel(OtcClient *otcClient, QObject* parent = nullptr);
   ~OTCRequestViewModel() override = default;

   int rowCount(const QModelIndex &parent = QModelIndex()) const override;
   int columnCount(const QModelIndex &parent = QModelIndex()) const override;
   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
   QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

   QModelIndex getIndexByTimestamp(QDateTime timeStamp);
public slots:
   void onRequestsUpdated();

private slots:
   void onUpdateDuration();

signals:
   void restoreSelectedIndex();

private:
   enum class Columns
   {
      Security,
      Type,
      Product,
      Side,
      Quantity,
      Duration,

      Latest = Duration,
   };

   struct OTCRequest
   {
      bs::network::otc::QuoteRequest request_;
      bool isOwnRequest_;
   };
   std::vector<OTCRequest> request_;

   OtcClient *otcClient_{};
   QTimer updateDurationTimer_;

};

#endif
