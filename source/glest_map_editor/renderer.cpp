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

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItemGroup>
#include <QAction>

//Qt needs this, otherwise glestlib will load winsock.h and break everything
#ifdef WIN32
    #include <winsock2.h>
#endif

#include "renderer.hpp"

//good idea?
#include "mainWindow.hpp"
#include "tile.hpp"
#include "player.hpp"
#include "selection.hpp"
#include "mapManipulator.hpp"

#include <iostream>

namespace MapEditor {
    Renderer::Renderer(MapManipulator *mapman, Status *status){
        this->water = true;
        this->heightMap = false;
        this->grid = false;
        this->status = status;
        this->filename = "";
        this->mapman = mapman;
        this->map = new Shared::Map::MapPreview();
        this->map->resetFactions(Shared::Map::DEFAULT_MAP_FACTIONCOUNT);//otherwise this map can’t be loaded later
        this->map->setHasChanged(false);//Don’t save blank maps :D
        this->scene = new QGraphicsScene();
        this->width = this->map->getW();
        this->height = this->map->getH();

        this->selectionRect = new Selection();
        this->selectionRect->resize(this->width,this->height);
        connect(scene, SIGNAL( selectionChanged() ), mapman, SLOT( selectionChanged() ));

        //mapman->setRenderer(this);
        this->tileContainer = new QGraphicsItemGroup();
        this->tileContainer->setHandlesChildEvents(false);
        this->scene->addItem(this->tileContainer);
        this->playerContainer = new QGraphicsItemGroup();
        this->playerContainer->setHandlesChildEvents(false);
        this->scene->addItem(this->playerContainer);
        this->scene->addItem(this->selectionRect);//must be on top

        //this->player = new Player[];{Player(Qt::black),Player(Qt::black),Player(Qt::black),Player(Qt::black),Player(Qt::black),Player(Qt::black),Player(Qt::black),Player(Qt::black)};
        this->player[0] = new Player(Qt::red);
        this->player[1] = new Player(Qt::blue);
        this->player[2] = new Player(Qt::green);
        this->player[3] = new Player(Qt::yellow);
        this->player[4] = new Player(Qt::white);
        this->player[5] = new Player(Qt::cyan);
        this->player[6] = new Player(QColor(0xFF,0xA5,0x00));//orange
        this->player[7] = new Player(QColor(0xFF,0xC0,0xCB));//pink
        for(unsigned int i = 0; i < (sizeof(player)/sizeof(*player)); i++){
            this->player[i]->setParentItem(this->playerContainer);
        }

        this->updatePlayerPositions();
        this->updateMaxPlayers();

        this->createTiles();

        this->recalculateAll();
        this->clearHistory();

        mapman->setRenderer(this);
    }

    Renderer::~Renderer(){
        this->removeTiles();
        delete this->tileContainer;
        for(unsigned int i = 0; i < (sizeof(player)/sizeof(*player)); i++){
            delete this->player[i];
        }
        delete this->playerContainer;
        delete this->selectionRect;
        delete this->map;
        delete this->mapman;
    }

    void Renderer::open(string path){
        this->filename = path;
        this->addHistory();
        this->map->loadFromFile(path);
        if(this->width == this->map->getW() && this->height == this->map->getH()){
            this->updateMap();
            this->recalculateAll();
        }else{
            this->resize();
        }
        this->updatePlayerPositions();
        this->updateMaxPlayers();
        this->mapman->selectAll();
        this->clearHistory();
    }

    //destroys the whole map, takes some time
    void Renderer::resize(){
        this->removeTiles();
        this->width = this->map->getW();
        this->height = this->map->getH();
        this->createTiles();
        this->recalculateAll();//TODO: do we need an .update() ?
        this->scene->setSceneRect(this->scene->itemsBoundingRect());
    }

    void Renderer::save(){
        if(this->isSavable()){//save as last saved or opened file
            this->save(this->filename);
        }
    }

    void Renderer::save(std::string path){
        this->filename = path;//maybe this is the first time we save/open
        std::cout << "save to " << path << std::endl;
        this->map->saveToFile(path);
    }

    bool Renderer::isSavable(){
        return this->filename != "";
    }

    std::string Renderer::getFilename() const{
        return this->filename;
    }

    void Renderer::resetFilename(){
        this->filename = "";
    }


    //‘recalculate’ which borders are necessary, if water is visible etc … for every tile
    void Renderer::recalculateAll(){
        for(int column = 0; column < this->width; column++){
            for(int row = 0; row < this->height; row++){
                this->Tiles[column][row]->recalculate();
            }
        }
    }

    //let all tiles update their cached height
    void Renderer::updateMap(){
        for(int column = 0; column < this->width; column++){
            for(int row = 0; row < this->height; row++){
                this->Tiles[column][row]->updateHeight();
            }
        }
    }


    int Renderer::getHeight() const{
        return this->height;
    }

    int Renderer::getWidth() const{
        return this->width;
    }

    Shared::Map::MapPreview *Renderer::getMap() const{
        return this->map;
    }

    MapManipulator *Renderer::getMapManipulator() const{
        return this->mapman;
    }

    QGraphicsScene *Renderer::getScene() const{
        return this->scene;
    }

    Tile *Renderer::at(int column, int row) const{
        return this->Tiles[column][row];
    }

    bool Renderer::exists(int column, int row) const{
        return (column >= 0 && row >= 0 && column < this->width && row < this->height);
    }

    Status *Renderer::getStatus() const{
        return status;
    }

    bool Renderer::getWater() const{
        return this->water;
    }

    bool Renderer::getGrid() const{
        return this->grid;
    }

    bool Renderer::getHeightMap() const{
        return this->heightMap;
    }

    Selection *Renderer::getSelectionRect(){
        return this->selectionRect;
    }

    void Renderer::setWater(bool water){
        this->water = water;
        this->recalculateAll();//new colors
    }

    void Renderer::setGrid(bool grid){
        this->grid = grid;
        this->updateTiles();
    }

    void Renderer::undo(){
        int realIndex = this->history.size() -1 - this->historyPos;
        //std::cout << "historyPos: " << historyPos << "; realIndex: " << realIndex << "; size: " << this->history.size() << std::endl;
        if(realIndex > 0){
            this->historyPos++;
            *this->map = this->history[realIndex-1];
            this->resize();
            this->updateMap();
            this->recalculateAll();
            //this->updateTiles();
            this->updatePlayerPositions();
            this->updateMaxPlayers();
            this->resize();
        }
    }

    void Renderer::redo(){
        //int realIndex = (this->history.size()-1) - this->historyPos;
        //std::cout << "historyPos: " << historyPos << "; realIndex: " << realIndex << "; size: " << this->history.size() << std::endl;
        if(this->historyPos > 0){
            this->historyPos--;
            int realIndex = (this->history.size()-1) - this->historyPos;
            *this->map = this->history[realIndex];
            this->resize();
            this->updateMap();
            this->recalculateAll();
            //this->updateTiles();
            this->updatePlayerPositions();
            this->updateMaxPlayers();

        }
    }

    void Renderer::forgetLast(){
        this->history.pop_back();
    }

    void Renderer::addHistory(){
        while(this->historyPos > 0){//1 is latest change; 0 is actual
            this->historyPos--;
            this->history.pop_back();
        }
        this->history.push_back(*this->map);//has no pointer, should work
    }

    void Renderer::clearHistory(){
        this->history.clear();
        this->historyPos = 0;//1 is previous change //0 would be actual map
        this->history.push_back(*this->map);
    }

    void Renderer::zoomIn(){
        zoom(1,4);
    }

    void Renderer::zoomOut(){
        zoom(-1,4);
    }

    void Renderer::fitZoom(){
        QGraphicsView* viewer = this->mapman->getWindow()->getView();
        viewer->resetTransform();
        QRect rect = viewer->frameRect();
        //getting ratios that fit them all
        int heightRatio = rect.height()/(double)this->height;
        int widthRatio = rect.width()/(double)this->width;
        int growBy = min(heightRatio,widthRatio) -Tile::getSize();
        if(growBy > 0){
            zoom(1,growBy);
        }
    }

    void Renderer::setHeightMap(bool heightMap){
        this->heightMap = heightMap;
        this->recalculateAll();
        this->updateTiles();//because of cliffs, would not be necessary if water got hidden
    }

    void Renderer::updateTiles(){
        for(int column = 0; column < this->width; column++){
            for(int row = 0; row < this->height; row++){
                this->Tiles[column][row]->update();
            }
        }
    }

    void Renderer::updatePlayerPositions(){
        for(unsigned int i = 0; i < (sizeof(player)/sizeof(*player)); i++){
            //cout << i << ": " << this->map->getStartLocationX(i) << ", " << this->map->getStartLocationY(i) << endl;
            this->player[i]->move(this->map->getStartLocationX(i), this->map->getStartLocationY(i));
        }
    }

    void Renderer::restorePlayerPositions(){
        for(unsigned int i = 0; i < (sizeof(player)/sizeof(*player)); i++){
            this->map->changeStartLocation(this->player[i]->getColumn(), this->player[i]->getRow(), i);
        }
    }

    void Renderer::updateMaxPlayers(){
        unsigned int maxPlayers = this->map->getMaxFactions();
        this->mapman->getWindow()->limitPlayers(maxPlayers);
        for(unsigned int i = 0; i < (sizeof(player)/sizeof(*player)); i++){
            this->player[i]->setVisible(i < maxPlayers);
        }
    }

    void Renderer::createTiles(){
        this->Tiles = new Tile**[this->width];
        for(int column = 0; column < this->width; column++){
            this->Tiles[column] = new Tile*[this->height];
            for(int row = 0; row < this->height; row++){
                this->Tiles[column][row] = new Tile(this->tileContainer, this, column, row);
            }
        }
    }

    void Renderer::removeTiles(){
        for(int column = 0; column < this->width; column++){
            for(int row = 0; row < this->height; row++){
                ///this->tileContainer->removeFromGroup(this->Tiles[column][row]);
                delete this->Tiles[column][row];
            }
            delete[] this->Tiles[column];
        }
        delete[] this->Tiles;
    }

    void Renderer::zoom(int delta, int pixels){
        QGraphicsView* viewer = this->mapman->getWindow()->getView();
        viewer->setTransformationAnchor(QGraphicsView::AnchorViewCenter);


        double oldScale = viewer->transform().m11();
        //each level has 2 further pixels
        double oldLevel = (((Tile::getSize() * oldScale) - Tile::getSize())/2);

        double px = ((2.0)/Tile::getSize());//scale for 2px
        if(delta > 0) {// zoom in
            if(oldLevel < 10*Tile::getSize()){
                double scale = 1.0+(oldLevel+pixels/2)*px;
                //create a new transformation matrix
                viewer->setTransform(QTransform().scale(scale, scale));
            }
        } else {// zoomout
            if(oldLevel > 1){
                double scale = 1.0+(oldLevel-pixels/2)*px;
                viewer->setTransform(QTransform().scale(scale, scale));
            }else{
                viewer->resetTransform();
            }
        }
    }

}
