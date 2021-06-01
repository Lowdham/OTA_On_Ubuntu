#include "update_module.hpp"

namespace otalib::app {
namespace {
using TempRequestCtrl =
    PrintCtrl<kHeadTagWarn, ' ', fgColor::Yellow, false, false>;
//
void TipsOutput(const RequestResponse<AppVersionType>& response) {
  //
  switch (::std::get<sAction>(response)) {
    case sAction::None:
      print<GeneralSuccessCtrl>(::std::cout,
                                "There's no further update or rollback action "
                                "according to server's strategy.");
      return;
    case sAction::Update: {
      auto stg = ::std::get<StrategyType>(response);
      QString format("Latest version detected.");
      if (stg == StrategyType::compulsory)
        format +=
            " This update is compulsory according to strategy stored in "
            "server. Your version is [%1]. Destination version is [%2].";
      else if (stg == StrategyType::optional)
        format +=
            " This update is optional according to strategy stored in "
            "server. Your version is [%1]. Destination version is "
            "[%2].\nContinue[y/n]:";
      // '3' is index of 'From'. '4' is index of 'Destination'
      format = format.arg(::std::get<2>(response).toString())
                   .arg(::std::get<3>(response).toString());
      print<TempRequestCtrl>(::std::cout, format);
      return;
    }
    case sAction::Rollback:
      auto stg = ::std::get<StrategyType>(response);
      QString format("Latest version detected.");
      if (stg == StrategyType::compulsory)
        format +=
            " This rollback is compulsory according to strategy stored in "
            "server. Your version is [%1]. Destination version is [%2].";
      else if (stg == StrategyType::optional)
        format +=
            " This rollback is optional according to strategy stored in "
            "server. Your version is [%1]. Destination version is "
            "[%2].\nContinue[y/n]:";
      // '3' is index of 'From'. '4' is index of 'Destination'
      format = format.arg(::std::get<2>(response).toString())
                   .arg(::std::get<3>(response).toString());
      print<TempRequestCtrl>(::std::cout, format);
      return;
  }
}
}  // namespace
UpdateModule::UpdateModule() noexcept : net_() {
  //
}

void UpdateModule::TryUpdate() {
  // Try to connect to the server.
  printf("\n");
  if (!net_.Connect()) {
    for (int i = 0; i < 5; ++i) {
      if (net_.Connect()) break;
      ::std::this_thread::sleep_for(::std::chrono::seconds(1));

      if (i == 4)  // Fail to connect.
        throw AppError(AppError::index_network_unfunctional);
    }
  }

  // Make update request.
  Property pp = ReadProperty();
  QJsonDocument jdoc = MakeRequest(pp.app_name_, pp.app_version_, pp.app_type_);
  // Send request to server.
  if (!net_.Send(jdoc.toJson())) {
    net_.Close();
    throw AppError(AppError::index_network_send_fail);
  }
  // Receive the server's response.
  if (!net_.Recv()) {
    net_.Close();
    throw AppError(AppError::index_network_recv_fail);
  }
  net_.Close();
  QByteArray response_raw = net_.GetData();
  auto response = ParseResponse<AppVersionType>(response_raw);
  if (!response) throw AppError(AppError::index_update_response_corrupt);

  auto [act, stg, from, dest] = *response;
  TipsOutput(*response);
  if (act == sAction::None) return;

  if (stg == StrategyType::compulsory) {
    doUpdate(*response);
  } else {
    // Receive the input.
    char in;
  L1:
    ::std::cin.get(in);
    printf("\n");
    if (in == 'Y' || in == 'y') {
      doUpdate(*response);
    } else if (in == 'N' || in == 'n') {
      QString info;
      if (act == sAction::Update)
        info += "Update ";
      else
        info += "Rollback ";
      info += "has been canceled.";
      print<GeneralWarnCtrl>(::std::cout, info);
      return;
    } else {
      print<TempRequestCtrl>(::std::cerr, "Invalid input. Please input again:");
      goto L1;
    }
  }
}

bool UpdateModule::doUpdate(const RequestResponse<AppVersionType>& response) {
  QDir temp_dir(kOtaTmpDir);
  if (!temp_dir.exists()) ::mkdir(kOtaTmpDir.toStdString().c_str(), 0755);
  // Try to connect to the server.
  if (!net_.Connect()) {
    for (int i = 0; i < 5; ++i) {
      if (net_.Connect()) break;
      ::std::this_thread::sleep_for(::std::chrono::seconds(1));

      if (i == 4)  // Fail to connect.
        throw AppError(AppError::index_network_unfunctional);
    }
  }

  QJsonDocument jdoc = MakeConfirm(response);
  if (!net_.Send(jdoc.toJson())) {
    net_.Close();
    throw AppError(AppError::index_network_send_fail);
  }

  if (!net_.Recv()) {
    net_.Close();
    throw AppError(AppError::index_network_recv_fail);
  }

  QByteArray data = net_.GetData();
  QFile archive(kOtaTmpFile);
  if (!archive.open(QFile::WriteOnly | QFile::Truncate))
    throw OTAError::S_file_open_fail{kOtaTmpFile, STRING_SOURCE_LOCATION};

  if (archive.write(data) != data.size())
    throw OTAError::S_file_write_fail{kOtaTmpFile, STRING_SOURCE_LOCATION};

  archive.close();
  tar_extract_archive_file_gzip(kOtaTmpFile, kOtaTmpDir);
  QFile::remove(kOtaTmpFile);
  applyPackOnApp<AppVersionType>(QDir::current(), QDir(kOtaTmpDir),
                                 QFileInfo(kPubkeyFile), true);
  return true;
}

}  // namespace otalib::app
