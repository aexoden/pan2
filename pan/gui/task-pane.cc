//* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#include <config.h>
extern "C" {
  #include <glib.h>
  #include <glib/gi18n.h>
  #include <gtk/gtk.h>
}
#include <pan/general/debug.h>
#include <pan/general/macros.h>
#include <pan/general/utf8-utils.h>
#include <pan/tasks/queue.h>
#include <pan/icons/pan-pixbufs.h>
#include "pad.h"
#include "render-bytes.h"
#include "task-pane.h"

enum
{
  COL_TASK_POINTER,
  COL_TASK_STATE,
  NUM_COLS
};


/**
***  Internal Utility
**/

typedef Queue::tasks_t tasks_t;

void
TaskPane :: get_selected_tasks_foreach (GtkTreeModel *model, GtkTreePath *, GtkTreeIter *iter, gpointer list_g)
{
  Task * task (0);
  gtk_tree_model_get (model, iter, COL_TASK_POINTER, &task, -1);
  static_cast<tasks_t*>(list_g)->push_back (task);
}

tasks_t
TaskPane :: get_selected_tasks () const
{
  tasks_t tasks;
  GtkTreeView * view (GTK_TREE_VIEW (_view));
  GtkTreeSelection * sel (gtk_tree_view_get_selection (view));
  gtk_tree_selection_selected_foreach (sel, get_selected_tasks_foreach, &tasks);
  return tasks;
}

/**
***  User Interactions
**/

namespace
{
  int
  find_task_index (GtkListStore * list, Task * task)
  {
    GtkTreeIter iter;
    int index (0);

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(list), &iter)) do
    {
      Task * test;
      gtk_tree_model_get (GTK_TREE_MODEL(list), &iter, COL_TASK_POINTER, &test, -1);
      if (test == task)
        return index;
      ++index;
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL(list), &iter));

    // not found
    return -1;
  }
}

void TaskPane :: online_toggled_cb (GtkToggleButton* b, Queue *queue)
{
  queue->set_online (gtk_toggle_button_get_active (b));
}
void TaskPane :: up_clicked_cb (GtkButton*, TaskPane* pane)
{
  pane->_queue.move_up (pane->get_selected_tasks());
}
void TaskPane :: down_clicked_cb (GtkButton*, TaskPane* pane)
{
  pane->_queue.move_down (pane->get_selected_tasks());
}
void TaskPane :: top_clicked_cb (GtkButton*, TaskPane* pane)
{
  pane->_queue.move_top (pane->get_selected_tasks());
}
void TaskPane :: bottom_clicked_cb (GtkButton*, TaskPane* pane)
{
  pane->_queue.move_bottom (pane->get_selected_tasks());
}
void TaskPane :: stop_clicked_cb (GtkButton*, TaskPane* pane)
{
  pane->_queue.stop_tasks (pane->get_selected_tasks());
}
void TaskPane :: delete_clicked_cb (GtkButton*, TaskPane* pane)
{
  pane->_queue.remove_tasks (pane->get_selected_tasks());
}
void TaskPane :: restart_clicked_cb (GtkButton*, TaskPane* pane)
{
  pane->_queue.restart_tasks (pane->get_selected_tasks());
}

/**
***  Display
**/

namespace
{
  typedef Queue::task_states_t task_states_t;
}

/***
****  Queue Listener
***/

void
TaskPane :: on_queue_tasks_added (Queue& queue, int index, int count)
{
  task_states_t states;
  queue.get_all_task_states (states);

  for (int i=0; i<count; ++i)
  {
    const int pos (index + i);
    Task * task (states.tasks[pos]);
    GtkTreeIter iter;
    gtk_list_store_insert (_store, &iter, pos);
    gtk_list_store_set (_store, &iter,
                        COL_TASK_POINTER, task,
                        COL_TASK_STATE, (int)states.get_state(task),
                        -1);
  }
}

void
TaskPane :: on_queue_task_removed (Queue&, Task& task, int index)
{
  const int list_index (find_task_index (_store, &task));
  assert (list_index == index);
  GtkTreeIter iter;
  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL(_store), &iter, NULL, index);
  gtk_list_store_remove (_store, &iter);
}

void
TaskPane :: on_queue_task_moved (Queue&, Task&, int new_index, int old_index)
{
  const int count (gtk_tree_model_iter_n_children (GTK_TREE_MODEL(_store), NULL));
  std::vector<int> v (count);
  for (int i=0; i<count; ++i) v[i] = i;
  if (new_index < old_index) {
    v.erase (v.begin()+old_index);
    v.insert (v.begin()+new_index, old_index);
  } else {
    v.erase (v.begin()+old_index);
    v.insert (v.begin()+new_index, old_index);
  }
  gtk_list_store_reorder (_store, &v.front());
}

void TaskPane :: on_queue_task_active_changed (Queue&, Task&, bool) { }
void TaskPane :: on_queue_connection_count_changed (Queue&, int) { }
void TaskPane :: on_queue_size_changed (Queue&, int, int) { }

void TaskPane :: on_queue_online_changed (Queue&, bool online)
{
  g_signal_handler_block (_online_toggle, _online_toggle_handler);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(_online_toggle), online);
  g_signal_handler_unblock (_online_toggle, _online_toggle_handler);

  gtk_image_set_from_stock (GTK_IMAGE(_online_image),
                            online ? GTK_STOCK_CONNECT : GTK_STOCK_DISCONNECT,
                            GTK_ICON_SIZE_BUTTON);
}

void
TaskPane :: update_status (const task_states_t& tasks)
{
  int queued_count (0);
  int stopped_count (0);
  int running_count (0);
  guint64 bytes (0);
  foreach_const (tasks_t, tasks.tasks, it)
  {
    Task * task (*it);
    const Queue::TaskState state (tasks.get_state (task));
    if (state == Queue::RUNNING || state == Queue::DECODING)
      ++running_count;
    else if (state == Queue::STOPPED)
      ++stopped_count;
    else if (state == Queue::QUEUED || state == Queue::QUEUED_FOR_DECODE)
      ++queued_count;

    if (state==Queue::RUNNING || state==Queue::QUEUED)
      bytes += task->get_bytes_remaining ();
  }

  // titlebar
  char buf[1024];
  if (stopped_count)
    g_snprintf (buf, sizeof(buf), _("Pan: Tasks (%d Queued, %d Running, %d Stopped)"), queued_count, running_count, stopped_count);
  else if (running_count || queued_count)
    g_snprintf (buf, sizeof(buf), _("Pan: Tasks (%d Queued, %d Running)"), queued_count, running_count);
  else 
    g_snprintf (buf, sizeof(buf), _("Pan: Tasks"));
  gtk_window_set_title (GTK_WINDOW(_root), buf);

  // status label
  const unsigned long task_count (running_count + queued_count);
  double KiBps (_queue.get_speed_KiBps ());
  int hours(0), minutes(0), seconds(0);
  if (task_count) {
    const double KiB ((double)bytes / 1024);
    unsigned long tmp (KiBps>0.01 ? ((unsigned long)(KiB / KiBps)) : 0);
    seconds = tmp % 60ul; tmp /= 60ul;
    minutes = tmp % 60ul; tmp /= 60ul;
    hours   = tmp;
  }
  g_snprintf (buf, sizeof(buf), _("%lu tasks, %s, %.1f KiBps, ETA %d:%02d:%02d"),
              task_count, render_bytes(bytes), KiBps, hours, minutes, seconds);
  std::string line (buf);

  const tasks_t tasks_selected (get_selected_tasks ());
  const unsigned long selected_count (tasks_selected.size());
  if (selected_count) {
    guint64 selected_bytes (0ul);
    foreach_const (tasks_t, tasks_selected, it)
      selected_bytes += (*it)->get_bytes_remaining ();
    g_snprintf (buf, sizeof(buf), _("%lu selected, %s"), selected_count, render_bytes(selected_bytes));
    line += '\n';
    line += buf;
  }

  gtk_label_set_text (GTK_LABEL(_status_label), line.c_str());
}

gboolean
TaskPane :: periodic_refresh_foreach (GtkTreeModel    * model,
                                      GtkTreePath     * ,
                                      GtkTreeIter     * iter,
                                      gpointer          states_gpointer)
{
  Task * task;
  int old_state;
  gtk_tree_model_get (model, iter, COL_TASK_POINTER, &task,
                                   COL_TASK_STATE, &old_state, -1);

  const task_states_t * states (static_cast<const task_states_t*>(states_gpointer));
  const int new_state (states->get_state (task));

  if (new_state != old_state)
    gtk_list_store_set (GTK_LIST_STORE(model), iter, COL_TASK_POINTER, task,
                                                     COL_TASK_STATE, new_state,
                                                     -1);


  return false; // keep iterating
}

gboolean
TaskPane :: periodic_refresh (gpointer pane_gpointer)
{
  TaskPane * pane (static_cast<TaskPane*>(pane_gpointer));
  task_states_t tasks;
  pane->_queue.get_all_task_states (tasks);
  gtk_tree_model_foreach (GTK_TREE_MODEL(pane->_store), periodic_refresh_foreach, &tasks);
  pane->update_status (tasks);
  gtk_widget_queue_draw (pane->_view);

  return true;
}

namespace
{
  void
  render_state (GtkTreeViewColumn *,
                GtkCellRenderer *rend,
                GtkTreeModel *model,
                GtkTreeIter *iter,
                Queue * queue)
  {
    Task * task (0);
    int state;
    gtk_tree_model_get (model, iter, COL_TASK_POINTER, &task,
                                     COL_TASK_STATE,  &state,
                                     -1);

    // build the state string
    const char * state_str (0);
    switch (state) {
      case Queue::RUNNING:  state_str = _("Running"); break;
      case Queue::DECODING: state_str = _("Decoding"); break;
      case Queue::QUEUED_FOR_DECODE: state_str = _("Queued for Decode"); break;
      case Queue::QUEUED:   state_str = _("Queued"); break;
      case Queue::STOPPED:  state_str = _("Stopped"); break;
      case Queue::REMOVING: state_str = _("Removing"); break;
      default: state_str = _("Unknown"); break;
    }

    // get the time remaining
    const unsigned long bytes_remaining (task->get_bytes_remaining ());
    double speed;
    int connections;
    queue->get_task_speed_KiBps (task, speed, connections);
    const unsigned long seconds_remaining (speed>0.001 ? (unsigned long)(bytes_remaining/(speed*1024)) : 0);
    int h(0), m(0), s(0);
    if (seconds_remaining > 0) {
      h = seconds_remaining / 3600;
      m = (seconds_remaining % 3600) / 60;
      s = seconds_remaining % 60;
    }

    // get the percent done
    const int percent (task->get_progress_of_100());

    // get the description
    const std::string description (task->describe ());

    std::string status (state_str);

    if (percent) {
      char buf[128];
      g_snprintf (buf, sizeof(buf), _("%d%% Done"), percent);
      status += " - ";
      status += buf;
    }
    if (state == Queue::RUNNING) {
      char buf[128];
      g_snprintf (buf, sizeof(buf), _("%d:%02d:%02d Remaining (%d @ %lu KiB/s)"), h, m, s, connections, (unsigned long)speed);
      status += " - ";
      status += buf;
    }

    TaskArticle * ta (dynamic_cast<TaskArticle*>(task));
    if (ta) {
      const Quark& save_path (ta->get_save_path());
      if (!save_path.empty()) {
        status += " - \"";
        status += save_path;
        status += '"';
      }
    }

    if (!queue->is_online()) {
      char buf[512];
      g_snprintf (buf, sizeof(buf), "[%s] ", _("Offline"));
      status.insert (0, buf);
    }

    char * str (0);
    if (state == Queue::RUNNING || state == Queue::DECODING)
      str = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>", description.c_str(), status.c_str());
    else
      str = g_markup_printf_escaped ("%s\n<small>%s</small>", description.c_str(), status.c_str());
    const std::string str_utf8 = clean_utf8 (str);
    g_object_set(rend, "markup", str_utf8.c_str(), "xpad", 10, "ypad", 5, NULL);
    g_free (str);
  }
}

/**
***
**/

namespace
{
  void delete_task_pane (gpointer pane_gpointer)
  {
    delete static_cast<TaskPane*>(pane_gpointer);
  }
}

TaskPane :: ~TaskPane ()
{
  _queue.remove_listener (this);
  g_source_remove (_update_timeout_tag);
}

namespace
{
  GtkWidget * add_button (GtkWidget   * box,
                                      const gchar * stock_id,
                                      GCallback     callback,
                                      gpointer      user_data)
  {
    GtkWidget * w = gtk_button_new_from_stock (stock_id);
    gtk_button_set_relief (GTK_BUTTON(w), GTK_RELIEF_NONE);
    if (callback)
      g_signal_connect (w, "clicked", callback, user_data);
    gtk_box_pack_start (GTK_BOX(box), w, false, false, 0);
    return w;
  }
}

TaskPane :: TaskPane (Queue& queue, Prefs& prefs): _queue(queue)
{
  _root = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  GtkWidget * w;

  GtkWidget * vbox = gtk_vbox_new (false, 0);

    GtkWidget * buttons = gtk_hbox_new (false, PAD_SMALL);

    w = _online_toggle = gtk_check_button_new ();
    _online_toggle_handler = g_signal_connect (w, "clicked", G_CALLBACK(online_toggled_cb), &queue);
    GtkWidget * i = _online_image = gtk_image_new ();
    GtkWidget * l = gtk_label_new_with_mnemonic (_("_Online"));
    GtkWidget * v = gtk_hbox_new (false, PAD);
    gtk_label_set_mnemonic_widget (GTK_LABEL(l), w);
    on_queue_online_changed (queue, queue.is_online());
    gtk_box_pack_start (GTK_BOX(v), i, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX(v), l, 0, 0, 0);
    gtk_container_add (GTK_CONTAINER(w), v);
    gtk_box_pack_start (GTK_BOX(buttons), w, false, false, 0);
    gtk_box_pack_start (GTK_BOX(buttons), gtk_vseparator_new(), 0, 0, 0);

    add_button (buttons, GTK_STOCK_GO_UP, G_CALLBACK(up_clicked_cb), this);
    add_button (buttons, GTK_STOCK_GOTO_TOP, G_CALLBACK(top_clicked_cb), this);
    gtk_box_pack_start (GTK_BOX(buttons), gtk_vseparator_new(), 0, 0, 0);
    add_button (buttons, GTK_STOCK_GO_DOWN, G_CALLBACK(down_clicked_cb), this);
    add_button (buttons, GTK_STOCK_GOTO_BOTTOM, G_CALLBACK(bottom_clicked_cb), this);
    gtk_box_pack_start (GTK_BOX(buttons), gtk_vseparator_new(), 0, 0, 0);
    w = add_button (buttons, GTK_STOCK_REDO, G_CALLBACK(restart_clicked_cb), this);
    gtk_widget_set_tooltip_text( w, _("Restart Tasks"));
    w = add_button (buttons, GTK_STOCK_STOP, G_CALLBACK(stop_clicked_cb), this);
    gtk_widget_set_tooltip_text( w, _("Stop Tasks"));
    w = add_button (buttons, GTK_STOCK_DELETE, G_CALLBACK(delete_clicked_cb), this);
    gtk_widget_set_tooltip_text( w, _("Delete Tasks"));
    gtk_box_pack_start (GTK_BOX(buttons), gtk_vseparator_new(), 0, 0, 0);
    w = add_button (buttons, GTK_STOCK_CLOSE, 0, 0);
    g_signal_connect_swapped (w, "clicked", G_CALLBACK(gtk_widget_destroy), _root);
    pan_box_pack_start_defaults (GTK_BOX(buttons), gtk_event_box_new()); // eat h space

  gtk_box_pack_start (GTK_BOX(vbox), buttons, false, false, 0);
  gtk_box_pack_start (GTK_BOX(vbox), gtk_hseparator_new(), false, false, 0);

  // statusbar
  GtkWidget * hbox = gtk_hbox_new (false, PAD);
  w = _status_label = gtk_label_new (0);
  gtk_box_pack_start (GTK_BOX(hbox), w, false, false, PAD_SMALL);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, false, false, PAD);

  _store = gtk_list_store_new (NUM_COLS, G_TYPE_POINTER, G_TYPE_INT);
  _view = gtk_tree_view_new_with_model (GTK_TREE_MODEL(_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(_view), false);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(_view), true);
  GtkCellRenderer * renderer = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn * col = gtk_tree_view_column_new_with_attributes (_("State"), renderer, NULL);
  gtk_tree_view_column_set_cell_data_func (col, renderer, (GtkTreeCellDataFunc)render_state, &_queue, 0);
  gtk_tree_view_append_column (GTK_TREE_VIEW(_view), col);
  GtkTreeSelection * selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(w), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER(w), _view);
  gtk_box_pack_start (GTK_BOX(vbox), w, true, true, 0);

  // populate the list
  task_states_t states;
  queue.get_all_task_states (states);
  foreach_r (tasks_t, states.tasks, it) {
    GtkTreeIter iter;
    gtk_list_store_prepend (_store, &iter);
    gtk_list_store_set (_store, &iter,
                        COL_TASK_POINTER, *it,
                        COL_TASK_STATE, (int)states.get_state(*it),
                        -1);
  }

  _queue.add_listener (this);

  _update_timeout_tag = g_timeout_add (750, periodic_refresh, this);

  gtk_window_set_role (GTK_WINDOW(_root), "pan-main-window");
  g_object_set_data_full (G_OBJECT(_root), "pane", this, delete_task_pane);
  gtk_container_add (GTK_CONTAINER(_root), vbox);
  gtk_widget_show_all (vbox);
  prefs.set_window ("tasks-window", GTK_WINDOW(_root), 200, 200, 550, 600);
  gtk_window_set_resizable (GTK_WINDOW(_root), true);
}
