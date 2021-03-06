// ==============================================================
//  This file is part of MegaGlest (www.megaglest.org)
//
//  Copyright (C) 2014 Sebastian Riedel
//
//  You can redistribute this code and/or modify it under
//  the terms of the GNU General Public License as published
//  by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version
// ==============================================================

#ifndef SELECTION_HPP
#define SELECTION_HPP

#include <QGraphicsItem>

namespace MapEditor {
    class Renderer;

    class Selection:public QGraphicsItem{
        public:
            Selection();
            //~Selection();
            virtual QRectF boundingRect() const;
            virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
            void move(int column, int row);
            void resize(int width, int height);
        private:
            int column;
            int row;
            int width;
            int height;
    };
}
#endif
