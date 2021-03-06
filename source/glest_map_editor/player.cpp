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

#include <QPainter>

#ifdef WIN32
    #include <winsock2.h>
#endif

#include "player.hpp"

#include "tile.hpp"
#include "renderer.hpp"

#include <iostream>

namespace MapEditor {

    //column and row are the position in Players of the Player
    Player::Player(const QColor &color):QGraphicsItem(){
        this->column = 0;
        this->row = 0;
        this->color = color;
        this->color.setAlphaF(.5);
    }

    QRectF Player::boundingRect() const{//need this for colidesWith … setPos is ignored
        return QRectF((column - 1) * Tile::getSize(),(row - 1) * Tile::getSize(), Tile::getSize()*3, Tile::getSize()*3);
    }

    void Player::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
        int size = Tile::getSize();
        painter->translate((column - 1) * size,(row - 1) * size);

        QPen color(QPen(this->color, 3, Qt::SolidLine, Qt::RoundCap));
        color.setCosmetic(true);//keep width when transformed
        painter->setPen(color);
        QLine lines[2] = {QLine(1,1,size*3 - 2,size*3 - 2),
                          QLine(1,size*3 - 2,size*3 - 2,1)};
        painter->drawLines(lines, 2);
        //drawLine would produce overlapping lines -> ugly with half opacity
        //don't use complete tiles
        //~ painter->eraseRect(0,0,size*3,2);
        //~ painter->eraseRect(0,0,2,size*3);
        //~ painter->eraseRect(0,size*3-2,size*3,2);
        //~ painter->eraseRect(size*3-2,0,2,size*3);
    }

    void Player::move(int column,int row){
        this->column = column;
        this->row = row;
        this->prepareGeometryChange();
    }

    int Player::getRow(){
        return row;
    }

    int Player::getColumn(){
        return column;
    }

}
