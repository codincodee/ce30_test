#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <ce30_driver/ce30_driver.h>
#include <QLabel>
#include <QElapsedTimer>
#include <fstream>

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

protected:
  void timerEvent(QTimerEvent* event);

private slots:
  void on_RecordPushButton_clicked();

private:
  Ui::MainWindow *ui;
  void PacketReceiveThread();
  static int ConnectOrExit(ce30_driver::UDPSocket& socket);
  std::shared_ptr<ce30_driver::UDPSocket> socket_;
  std::unique_ptr<std::thread> thread_;
  std::mutex scan_mutex_;
  ce30_driver::Scan scan_;
  std::mutex signal_mutex_;
  bool kill_signal_;
  std::condition_variable condition_;

  std::vector<QLabel*> labels_;
  std::vector<float> dist_buffer_;
  std::vector<float> amp_buffer_;
  const int focus_width_;
  const int focus_height_;
  QElapsedTimer elapsed_;
  QElapsedTimer sec_elapsed_;
  int frame_counter_;
  int sec_counter_;
  std::ofstream os_;
  std::ofstream record_os_;
};

#endif // MAIN_WINDOW_H
