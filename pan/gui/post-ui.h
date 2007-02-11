/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Pan - A Newsreader for Gtk+
 * Copyright (C) 2002-2006  Charles Kerr <charles@rebelbase.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __PostUI_h__
#define __PostUI_h__

#include <gmime/gmime-message.h>
#include <pan/gui/prefs.h>
#include <pan/general/progress.h>
#include <pan/tasks/queue.h>
#include <pan/usenet-utils/text-massager.h>
#include "group-prefs.h"

namespace pan
{
  class Profiles;
  class TaskPost;

  /**
   * Dialog for posting new messages Pan's GTK GUI.
   * @ingroup GUI
   */
  class PostUI: private Progress::Listener
  {
    public:
      static PostUI* create_window (GtkWindow*, Data&, Queue&, GroupServer&, Profiles&,
                                    GMimeMessage*, Prefs&, GroupPrefs&);
    
    protected:
      PostUI (GtkWindow*, Data&, Queue&, GroupServer&, Profiles&,
              GMimeMessage*, Prefs&, GroupPrefs&);
    public:
      ~PostUI ();

    public:
      GtkWidget * root() { return _root; }
      void rot13_selection ();
      void wrap_body ();
      void spawn_editor ();
      void manage_editors ();
      void manage_profiles ();
      void set_editor_command (const StringView&);
      void set_charset (const StringView&);
      void apply_profile ();
      void save_draft ();
      void open_draft ();
      void send_now ();
      void close_window ();
      void set_wrap_mode (bool wrap);

    private:
      void done_sending_message (GMimeMessage*, bool);
      void maybe_mail_message (GMimeMessage*);
      bool maybe_post_message (GMimeMessage*);

    private:
      void add_editors_to_menu ();
      void update_widgetry ();
      void update_profile_combobox ();
      void populate_from_message (GMimeMessage*);
      void set_message (GMimeMessage*);

    private:
      virtual void on_progress_finished (Progress&, int status);
      virtual void on_progress_error (Progress&, const StringView&);

    private:
      Data& _data;
      Queue& _queue;
      GroupServer& _gs;
      Profiles& _profiles;
      Prefs& _prefs;
      GroupPrefs& _group_prefs;
      GtkWidget * _root;
      GtkWidget * _from_combo;
      GtkWidget * _subject_entry;
      GtkWidget * _groups_entry;
      GtkWidget * _to_entry;
      GtkWidget * _body_view;
      GtkTextBuffer * _body_buf;
      GtkTextBuffer * _headers_buf;
      GMimeMessage * _message;
      std::string _charset;
      TextMassager _tm;
      std::set<unsigned int> _editor_ui_ids;
      GtkUIManager * _uim;
      GtkActionGroup * _agroup;
      std::string _signature;
      GtkWidget * _post_dialog;
      TaskPost * _post_task;
      typedef std::map<std::string, std::string> str2str_t;
      str2str_t _hidden_headers;
      str2str_t _profile_headers;
      std::string _unchanged_body;

    private:
      void add_actions (GtkWidget* box);
      void add_editor_list ();
      void add_charset_list ();
      void apply_profile_to_body ();
      void apply_profile_to_headers ();
      enum Mode { DRAFTING, POSTING };
      GMimeMessage * new_message_from_ui (Mode mode);
      bool check_message (const Quark& server, GMimeMessage*);
      bool check_charset ();

    private:
      std::string get_body () const;

    private:
      unsigned long _group_entry_changed_id;
      unsigned int _group_entry_changed_idle_tag;
      static gboolean group_entry_changed_idle (gpointer);
      static void group_entry_changed_cb (GtkEditable*, gpointer);
      static void charset_selected_cb_static (GtkRadioAction*, GtkRadioAction*, gpointer);
      void charset_selected_cb (const char * charset);

    public:
      void set_spellcheck_enabled (bool);

  };
}

#endif
