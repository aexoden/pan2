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

#ifndef __MemChunk_h__
#define __MemChunk_h__

namespace pan {
  template <class T> class MemChunk
  {
    public:
      void push_back(const T& src)
      {
        if (head==0) grow();
        phead=reinterpret_cast<T*>(head);
        head=head->next;
        new(phead) T(src);
      }
      T& back()
      {
        return *phead;
      }
      
      MemChunk():chunks(0),head(0),phead(0) {};
      ~MemChunk()
      {
        Chunk *p=chunks;
        while(chunks!=0)
        {
          chunks=chunks->next;
          delete p;
          p=chunks;
        }
      }
      
    
    private:
      template<class U> MemChunk(MemChunk<U>&);
      MemChunk* operator=(const MemChunk&);
      
      struct Link {Link* next;};
      struct Chunk {
        enum {size=8*1024-sizeof(Chunk*)};
        char mem[size];
        Chunk *next;
      };

      void grow()
      {
        const int nelem=Chunk::size/sizeof(T);
        Chunk *c=new Chunk;
        char *p,*n=0;
        int i;
        
        for (p=c->mem, i=0;i<nelem-1;i++)
        {
          n=p+sizeof(T);
          reinterpret_cast<Link*>(p)->next=reinterpret_cast<Link*>(n);
          p=n;
        }
        reinterpret_cast<Link*>(p)->next=0;
        
        c->next=chunks;
        chunks=c;
        head=reinterpret_cast<Link*>(c->mem);
      };
      
      Chunk *chunks;
      Link *head;
      T *phead;
  };

}
#endif
