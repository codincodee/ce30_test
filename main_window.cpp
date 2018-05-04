#include "main_window.h"
#include "ui_main_window.h"
#include <QLabel>
#include <QDebug>
#include <string>
#include <iostream>
#include <QThread>

using namespace std;
using namespace ce30_driver;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  kill_signal_(false),
  focus_height_(6),
  focus_width_(6),
  frame_counter_(0),
  sec_counter_(0)
{
  ui->setupUi(this);
  for (int x = 0; x < focus_width_; ++x) {
    for (int y = 0; y < focus_height_; ++y) {
      QLabel* label = new QLabel(QString::number(x) + "," + QString::number(y));
      labels_.push_back(label);
      label->setAlignment(Qt::AlignCenter);
      ui->PixelGridLayout->addWidget(label, x, y);

      dist_buffer_.push_back(0.0f);
      amp_buffer_.push_back(0.0f);
    }
  }
  startTimer(0);
  elapsed_.start();
  sec_elapsed_.start();
  os_.open("time.txt");
}

MainWindow::~MainWindow()
{
  delete ui;
  signal_mutex_.lock();
  kill_signal_ = true;
  signal_mutex_.unlock();

  if (thread_ && thread_->joinable()) {
    thread_->join();
  }
  StopRunning(*socket_);
  os_.close();
}

int MainWindow::ConnectOrExit(UDPSocket& socket) {
  if (!Connect(socket)) {
    cerr << "Unable to Connect Device!" << endl;
    return 1;
  }
  string device_version;
  if (!GetVersion(device_version, socket)) {
    cerr << "Unable to Retrieve CE30 Device Version" << endl;
    return 1;
  }
  cout << "CE30 Version: " << device_version << endl;
  if (!StartRunning(socket)) {
    cerr << "Unable to Start CE30" << endl;
    return 1;
  }
  return 0;
}

void MainWindow::timerEvent(QTimerEvent *event) {
  if (!socket_) {
    socket_.reset(new UDPSocket);
    auto ec = ConnectOrExit(*socket_);
    if (ec != 0) {
      QThread::sleep(2);
      QCoreApplication::exit((int)ec);
      return;
    }
    if (!thread_) {
      thread_.reset(
          new std::thread(bind(&MainWindow::PacketReceiveThread, this)));
    }
  }

  static Scan scan;
  std::unique_lock<std::mutex> lock(scan_mutex_);
  condition_.wait(lock);
  scan = scan_;
  lock.unlock();

  if (!scan.Ready()) {
    return;
  }
  ++frame_counter_;

  int scan_x, scan_y, index;
  for (int x = 0; x < focus_width_; ++x) {
    for (int y = 0; y < focus_height_; ++y) {
      scan_x = Scan::Width() / 2 - focus_width_ / 2 + x;
      scan_y = Scan::Height() / 2 - focus_height_ / 2 + y;
      index = x * focus_width_ + y;
      dist_buffer_[index] = scan.at(scan_x, scan_y).distance;
      amp_buffer_[index] = scan.at(scan_x, scan_y).amplitude;
    }
  }
  if (record_os_.is_open()) {
    for (int i = 0; i < dist_buffer_.size(); ++i) {
      record_os_ << dist_buffer_[i] << " " << amp_buffer_[i] << " ";
    }
    record_os_ << "\n";
  }

  if (sec_elapsed_.elapsed() >= 1000) {
    auto elapsed = elapsed_.elapsed();
    ui->ElapsedLabel->setText(QString::number(elapsed));
    float freq = frame_counter_ / (sec_elapsed_.elapsed() * 1.0f / 1000);
    ui->FrequencyLabel->setText(QString::number(freq));
    sec_elapsed_.start();
    frame_counter_ = 0;
    os_ << ++sec_counter_ << " " << freq << "\n";

    for (int i = 0; i < dist_buffer_.size(); ++i) {
      labels_[i]->setText(QString::number(dist_buffer_[i]));
    }
  }
}

void MainWindow::PacketReceiveThread() {
  while (true) {
    // signal_mutex_.lock();
    auto kill_signal = kill_signal_;
    // signal_mutex_.unlock();
    if (kill_signal) {
      return;
    }

    static Packet packet;
    static Scan scan;
    while (!scan.Ready()) {
      if (GetPacket(packet, *socket_, true)) {
        auto parsed = packet.Parse();
        if (parsed) {
          scan.AddColumnsFromPacket(*parsed);
        } else {
          cerr << "Error parsing package." << endl;
        }
      } else {
        cerr << "Error getting package." << endl;
      }
    }
    unique_lock<mutex> lock(scan_mutex_);
    scan_ = scan;
    condition_.notify_all();
    lock.unlock();
    scan.Reset();
  }
}

void MainWindow::on_RecordPushButton_clicked()
{
  if (ui->RecordPushButton->text() == "Record") {
    record_os_.open("data.txt");
    if (record_os_.is_open()) {
      ui->RecordPushButton->setText("Stop");
    }
  } else {
    ui->RecordPushButton->setText("Record");
    record_os_.close();
  }
}
