#ifndef SHELL_CMD_HPP
#define SHELL_CMD_HPP

#include <QString>
#include <string>

#if defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#endif  // !defined(_WIN64) && !defined(_WIN32)

namespace otalib {

static void tar_create_archive_file_gzip(const QString& directory,
                                         const QString& archive_file) {
  QString cmd =
      QString("tar -zcf \"%1\" \"%2\"").arg(archive_file).arg(directory);
  ::system(cmd.toStdString().c_str());
}

static void tar_extract_archive_file_gzip(const QString& archive_file,
                                          const QString& directory) {
  QString cmd =
      QString("tar -zxf \"%1\" \"%2\"").arg(archive_file).arg(directory);
  ::system(cmd.toStdString().c_str());
}

static void copyDir(const QString& source, const QString& dest) {
#if defined(_WIN64) || defined(_WIN32)
  QString s = "\"" + source + "\"";
  QString d = "\"" + dest + "\"";
  QString cmd("xcopy " + s + " " + d + " /S");
  system(cmd.toStdString().c_str());
  return;
#endif

#ifdef __linux__
  QString src = source.trimmed();
  if (src.isEmpty()) return;
  QString cmd("cp -r " + src + "/* " + dest);
  system(cmd.toStdString().c_str());
  return;
#endif
}

static ::std::string getCmdResult(const ::std::string& cmd) {
#ifdef __linux__
  if (cmd.empty()) return ::std::string();

  FILE* pipe = NULL;
  if ((pipe = popen(cmd.c_str(), "r")) == NULL) return ::std::string();

  ::std::string ret;
  char buffer[1024 * 8] = {0};
  while (fgets(buffer, sizeof(buffer), pipe)) ret += buffer;
  pclose(pipe);

  uint32_t size = ret.size();
  if (size > 0 && ret[size - 1] == '\n') ret = ret.substr(0, size - 1);
  return ret;
#endif  // !__linux__
#if defined(_WIN64) || defined(_WIN32)
  if (cmd.empty()) return ::std::string();

  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
  HANDLE hRead, hWrite;
  if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return ::std::string();

  STARTUPINFO si = {sizeof(STARTUPINFO)};
  GetStartupInfo(&si);
  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  si.wShowWindow = SW_HIDE;
  si.hStdError = hWrite;
  si.hStdOutput = hWrite;

  PROCESS_INFORMATION pi;
  ::std::wstring wstr(cmd.size(), '\0');
  mbstowcs(wstr.data(), cmd.c_str(), cmd.size());
  if (!CreateProcess(NULL, wstr.data(), NULL, NULL, TRUE, NULL, NULL, NULL, &si,
                     &pi))
    return ::std::string();

  CloseHandle(hWrite);

  std::string strRet;
  char buff[1024 * 8] = {0};
  DWORD dwRead = 0;
  while (ReadFile(hRead, buff, 1024, &dwRead, NULL))
    strRet.append(buff, dwRead);

  CloseHandle(hRead);

  return strRet;
#endif  // !defined(_WIN64) && !defined(_WIN32)
}

}  // namespace otalib

#endif  // SHELL_CMD_HPP
