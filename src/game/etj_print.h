/*
 * MIT License
 *
 * Copyright (c) 2023 ETJump team <zero@etjump.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
#include <string>

#include "etj_log.h"
#include "etj_shared.h"
#include "etj_string_utilities.h"
#include "g_local.h"

namespace ETJump {
class Print {
public:
  static constexpr char accentColor{'3'};
  // -1 so we can always fit the extra newline
  static constexpr int bytesPerPacket = BYTES_PER_PACKET - 1;
  static Log logger;

  template <typename... Targs>
  static void serverConsoleR(const std::string &format, const Targs &...Fargs) {

  }

  template <typename... Targs>
  static void serverConsole(const std::string &context,
                            const std::string &format,
                            const Targs &...Fargs) {

  }

  /**
   * Player console
   */
  template <typename... Targs>
  static void playerConsoleR(int clientNum, const std::string &format,
                             const Targs &...Fargs) {
    send(clientNum, "print", format, true, Fargs...);
  }

  template <typename... Targs>
  static void playerConsole(int clientNum, const std::string &context,
                            const std::string &format, const Targs &...Fargs) {
    playerConsoleR(clientNum, getContextMessage(context, format), Fargs...);
  }

  template <typename... Targs>
  static void playerConsole(gclient_t *client, const std::string &context,
                            const std::string &format, const Targs &...Fargs) {
    playerConsole(getClientNum(client), context, format, Fargs...);
  }

  template <typename... Targs>
  static void playerConsole(gentity_t *ent, const std::string &context,
                            const std::string &format, const Targs &...Fargs) {
    playerConsole(getClientNum(ent), context, format, Fargs...);
  }

  /**
   * Chat
   */
  template <typename... Targs>
  static void chatR(int clientNum, const std::string &format,
                    const Targs &...Fargs) {
    send(clientNum, "chat", format, true, Fargs...);
  }

  template <typename... Targs>
  static void chat(int clientNum, const std::string &context,
                   const std::string &format, const Targs &...Fargs) {
    chatR(clientNum, getContextMessage(context, format), Fargs...);
  }

  template <typename... Targs>
  static void chat(gclient_t *client, const std::string &context,
                   const std::string &format, const Targs &...Fargs) {
    chat(getClientNum(client), context, format, Fargs...);
  }

  template <typename... Targs>
  static void chat(gentity_t *ent, const std::string &context,
                   const std::string &format, const Targs &...Fargs) {
    chat(getClientNum(ent), context, format, Fargs...);
  }

  /**
   * Popup
   */
  template <typename... Targs>
  static void popupR(int clientNum, const std::string &format,
                     const Targs &...Fargs) {
    send(clientNum, "cpm", format, true, Fargs...);
  }

  template <typename... Targs>
  static void popup(int clientNum, const std::string &context,
                    const std::string &format, const Targs &...Fargs) {
    popupR(clientNum, getContextMessage(context, format), Fargs...);
  }

  template <typename... Targs>
  static void popup(gclient_t *client, const std::string &context,
                    const std::string &format, const Targs &...Fargs) {
    popup(getClientNum(client), context, format, Fargs...);
  }

  template <typename... Targs>
  static void popup(gentity_t *ent, const std::string &context,
                    const std::string &format, const Targs &...Fargs) {
    popup(getClientNum(ent), context, format, Fargs...);
  }

  /**
   * Banner
   */
  template <typename... Targs>
  static void bannerR(int clientNum, const std::string &format,
                      const Targs &...Fargs) {
    send(clientNum, "bp", format, true, Fargs...);
  }

  template <typename... Targs>
  static void banner(int clientNum, const std::string &context,
                     const std::string &format, const Targs &...Fargs) {
    bannerR(clientNum, getContextMessage(context, format), Fargs...);
  }

  template <typename... Targs>
  static void banner(gclient_t *client, const std::string &context,
                     const std::string &format, const Targs &...Fargs) {
    banner(getClientNum(client), context, format, Fargs...);
  }

  template <typename... Targs>
  static void banner(gentity_t *ent, const std::string &context,
                     const std::string &format, const Targs &...Fargs) {
    banner(getClientNum(ent), context, format, Fargs...);
  }

  /**
   * Center
   */
  template <typename... Targs>
  static void centerR(int clientNum, const std::string &format,
                      const Targs &...Fargs) {
    send(clientNum, "cp", format, true, Fargs...);
  }

  template <typename... Targs>
  static void center(int clientNum, const std::string &context,
                     const std::string &format, const Targs &...Fargs) {
    centerR(clientNum, getContextMessage(context, format), Fargs...);
  }

  template <typename... Targs>
  static void center(gclient_t *client, const std::string &context,
                     const std::string &format, const Targs &...Fargs) {
    center(getClientNum(client), context, format, Fargs...);
  }

  template <typename... Targs>
  static void center(gentity_t *ent, const std::string &context,
                     const std::string &format, const Targs &...Fargs) {
    center(getClientNum(ent), context, format, Fargs...);
  }


  template <typename... Targs>
  static void send(opt<int> clientNum, const std::string &command,
                   const std::string &formatString, bool checkForNewline,
                   const Targs &...Fargs) {
    try {
      auto message = stringFormat(formatString, Fargs...);

      if (clientNum.hasValue() && (clientNum.value() < CONSOLE_CLIENT_NUMBER ||
          clientNum.value() > MAX_CLIENTS)) {
        logger.error("Trying to send a message to clientNum %d: %s. Ignoring "
                     "the message.",
                     clientNum.value(), message);
        return;
      }

      auto splits = wrapWords(message, '\n', bytesPerPacket);

      if (splits.size() == 0) {
        return;
      }

      if (checkForNewline) {
        auto &lastPart = splits[splits.size() - 1];

        if (lastPart.length() > 0 && lastPart[lastPart.length() - 1] != '\n') {
          lastPart += "\n";
        }
      }

      for (const auto &split : splits) {
        if (!clientNum.hasValue()) {
          trap_SendServerCommand(
              -1,
              stringFormat("%s \"%s\"", command, split).c_str());
          G_Printf("%s", split.c_str());
        } else {
          if (clientNum.value() == CONSOLE_CLIENT_NUMBER) {
            G_Printf("%s", split.c_str());
          } else {
            trap_SendServerCommand(
                clientNum.value(),
                stringFormat("%s \"%s\"", command, split).c_str());
          }
        }
      }
    } catch (const std::runtime_error &e) {
      logger.error("tried to print `%s` but it failed due to: %s", formatString,
                   e.what());
    }
  }

  // Broadcast for playerConsole "print"
  template <typename... Targs>
  static void broadcastPlayerConsoleR(const std::string &format,
                                      const Targs &...Fargs) {
    send(opt<int>(), "print", format, true, Fargs...);
  }

  template <typename... Targs>
  static void broadcastPlayerConsole(const std::string &context,
                                     const std::string &format,
                                     const Targs &...Fargs) {
    broadcastPlayerConsoleR(getContextMessage(context, format), Fargs...);
  }

  // Broadcast for chat "chat"
  template <typename... Targs>
  static void broadcastChatR(const std::string &format, const Targs &...Fargs) {
    send(opt<int>(), "chat", format, true, Fargs...);
  }

  template <typename... Targs>
  static void broadcastChat(const std::string &context,
                            const std::string &format, const Targs &...Fargs) {
    broadcastChatR(getContextMessage(context, format), Fargs...);
  }

  // Broadcast for popup "cpm"
  template <typename... Targs>
  static void broadcastPopupR(const std::string &format,
                              const Targs &...Fargs) {
    send(opt<int>(), "cpm", format, true, Fargs...);
  }

  template <typename... Targs>
  static void broadcastPopup(const std::string &context,
                             const std::string &format, const Targs &...Fargs) {
    broadcastPopupR(getContextMessage(context, format), Fargs...);
  }

  // Broadcast for banner "bp"
  template <typename... Targs>
  static void broadcastBannerR(const std::string &format,
                               const Targs &...Fargs) {
    send(opt<int>(), "bp", format, true, Fargs...);
  }

  template <typename... Targs>
  static void broadcastBanner(const std::string &context,
                              const std::string &format,
                              const Targs &...Fargs) {
    broadcastBannerR(getContextMessage(context, format), Fargs...);
  }

  // Broadcast for center "cp"
  template <typename... Targs>
  static void broadcastCenterR(const std::string &format,
                               const Targs &...Fargs) {
    send(opt<int>(), "cp", format, true, Fargs...);
  }

  template <typename... Targs>
  static void broadcastCenter(const std::string &context,
                              const std::string &format,
                              const Targs &...Fargs) {
    broadcastCenterR(getContextMessage(context, format), Fargs...);
  }


private:
  static int getClientNum(gentity_t *ent);
  static int getClientNum(gclient_t *client);
  static std::string getContextMessage(const std::string &context,
                                       const std::string &formatString);
};
}
