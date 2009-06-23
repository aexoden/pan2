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

#include <config.h>
#include <cstdio>
#include <cctype>
#include <algorithm>
#include <glib.h>
#include <glib/gprintf.h>
#include <pan/general/string-view.h>
#include "numbers.h"

using namespace pan;

/*****
******  MARK (PRIVATE)
*****/

namespace
{
   typedef Numbers::ranges_t::iterator r_it;

   typedef Numbers::ranges_t::const_iterator r_cit;

   bool
   maybe_merge_ranges (Numbers::ranges_t & ranges, Numbers::ranges_t::size_type low)
   {
      bool merged = false;

      if (0<=low && low+2<=ranges.size())
      {
         Numbers::Range & r1 = ranges[low];
         Numbers::Range & r2 = ranges[low+1];
         if (r1.high+1 == r2.low)
         {
            r2.low = r1.low;
            ranges.erase (ranges.begin() + low);
            merged = true;
         }
      }

      return merged;
   }
};

/**
 * @return the number of articles newly-marked as read 
 */
uint64_t
Numbers :: mark_range (const Range& rr)
{
   ranges_t::size_type i;
   uint64_t retval = 0;
   bool range_found = false;
   ranges_t::size_type low_index (std::lower_bound (_marked.begin(), _marked.end(), rr.low)
                  - _marked.begin());
   ranges_t::size_type high_index (std::lower_bound (_marked.begin(), _marked.end(), rr.high)
                   - _marked.begin());

   retval = rr.high + 1 - rr.low;

   for (i=low_index; i<=high_index && i<_marked.size(); ++i)
   {
      Range& r = _marked[i];

      if (rr.contains (r)) /* read range is engulfed; remove */
      {
         retval -= r.high+1 - r.low; /* remove r from retval */
         _marked.erase (_marked.begin() + i);
         --high_index;
         --i;
      }
      else if (r.contains (rr)) /* no-op */
      {
         retval = 0;
         range_found = true;
      }
      else if (r.contains (rr.high)) /* change low */
      {
         Range * prev = !i ? NULL : &_marked[i-1];
         range_found = true;
         retval -= rr.high+1 - r.low; /* remove intersection from retval */
         r.low = prev ? std::max(rr.low, prev->high+1) : rr.low;
      }
      else if (r.contains (rr.low)) /* change high */
      {
         Range * next = i==_marked.size()-1 ? NULL : &_marked[i+1];
         range_found = true;
         retval -= r.high+1 - rr.low; /* remove intersection from retval */
         r.high = next ? std::min(rr.high, next->low-1) : rr.high;
      }
   }

   if (!range_found)
   {
      _marked.insert (_marked.begin()+low_index, rr);
      if (low_index!=0) --low_index;
      ++high_index;
   }

   for (i=low_index; i<=high_index && i<_marked.size(); )
   {
      if (maybe_merge_ranges (_marked, i))
         --high_index;
      else
         ++i;
   }

   return retval;
}

uint64_t
Numbers :: unmark_range (const Range& ur)
{
   uint64_t retval = 0;
   ranges_t::size_type i;
   ranges_t::size_type low_index (std::lower_bound (_marked.begin(), _marked.end(), ur.low)
                  - _marked.begin());
   ranges_t::size_type high_index (std::lower_bound (_marked.begin(), _marked.end(), ur.high)
                   - _marked.begin ());

   for (i=low_index; i<=high_index && i<_marked.size(); )
   {
      Range& r (_marked[i]);
      if (ur.contains (r)) // remove
      {
         retval += (r.high+1) - r.low;
         _marked.erase (_marked.begin() + i);
         --high_index;
      }
      else if (r.low!=ur.low && r.high!=ur.high && r.contains(ur)) // split
      {
         const Range range (ur.high+1, r.high);
         r.high = ur.low-1;
         retval += ur.high+1-ur.low;
         _marked.insert (_marked.begin()+low_index+1, range);
         ++high_index;
         i += 2;
      }
      else if (ur.low!=r.low && r.contains(ur.low)) // change high
      {
         retval += r.high+1 - ur.low;
         r.high = ur.low-1;
         ++i;
      }
      else if (ur.high!=r.high && r.contains(ur.high)) // change low
      {
         retval += ur.high+1 - r.low;
         r.low = ur.high+1;
         ++i;
      }
      else ++i;
   }

   return retval;
}

/*****
******  MARK (PUBLIC)
*****/

uint64_t
Numbers :: mark_range (uint64_t low, uint64_t high, bool add)
{
   const Range r (low, high);
   return add ? mark_range(r) : unmark_range(r);
}

bool
Numbers :: mark_one (uint64_t number, bool add)
{
   const uint64_t changed_qty (mark_range (number, number, add));

   if (add)
      return changed_qty==0;
   else /* remove */
      return changed_qty!=0;
}

void
Numbers :: mark_str (const StringView& str, bool add)
{
   StringView phigh, p(str);

   while (p.pop_token (phigh, ','))
   {
      StringView plow;
      phigh.pop_token (plow, '-');
      plow.trim ();
      phigh.trim ();
      const uint64_t low (plow.empty() ? 0 : g_ascii_strtoull (plow.str, NULL, 10));
      const uint64_t high (phigh.empty() ? low : g_ascii_strtoull (phigh.str, NULL, 10));
      mark_range (low, high, add);
   }
}

void
Numbers :: clear ()
{
   _marked.clear ();
}

/*****
******
*****/

void
Numbers :: clip (uint64_t low, uint64_t high)
{
   r_it it = std::lower_bound (_marked.begin(), _marked.end(), low);
   _marked.erase (_marked.begin(), it);
   if (!_marked.empty() && _marked.front().contains(low))
      _marked.front().low = low;

   it = std::lower_bound (_marked.begin(), _marked.end(), high);
   if (it != _marked.end())
      _marked.erase (it+1, _marked.end());
   if (!_marked.empty() && _marked.back().contains(high))
      _marked.back().high = high;
}

/*****
******
*****/

bool
Numbers :: is_marked (uint64_t number) const
{
   r_cit it = std::lower_bound (_marked.begin(), _marked.end(), number);

   return it!=_marked.end() && it->contains(number);
}


std::string
Numbers :: to_string () const
{
  std::string tmp;
  to_string (tmp);
  return tmp;
}

void
Numbers :: to_string (std::string & str) const
{
   char buf[64];

   for (r_cit it=_marked.begin(), end=_marked.end(); it!=end; ++it)
   {
      Range r (*it);

      if (r.low == r.high)
        g_snprintf (buf, sizeof(buf), "%llu,", r.low);
      else
         g_snprintf (buf, sizeof(buf), "%llu-%llu,", r.low,r.high);
      str += buf;
   }

   if (!str.empty())
      str.erase (--str.end()); // remove final comma
}
