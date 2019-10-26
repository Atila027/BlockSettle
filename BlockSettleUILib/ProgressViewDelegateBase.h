#ifndef __PROGRESS_VIEW_DELEGATE_BASE__
#define __PROGRESS_VIEW_DELEGATE_BASE__

#include <QStyledItemDelegate>
#include <QProgressBar>

class ProgressViewDelegateBase : public QStyledItemDelegate
{
   Q_OBJECT

public:
   explicit ProgressViewDelegateBase(QWidget* parent = nullptr);
   ~ProgressViewDelegateBase() override = default;

   void paint(QPainter* painter, const QStyleOptionViewItem& opt,
      const QModelIndex& index) const override;

protected:
   virtual bool isDrawProgressBar(const QModelIndex& index) const = 0;
   virtual int maxValue(const QModelIndex& index) const = 0;
   virtual int currentValue(const QModelIndex& index) const = 0;

private:
   QProgressBar pbar_;
};


#endif // __PROGRESS_VIEW_DELEGATE_BASE__
