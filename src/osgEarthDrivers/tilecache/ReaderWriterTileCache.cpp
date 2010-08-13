/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
 * Copyright 2008-2010 Pelican Mapping
 * http://osgearth.org
 *
 * osgEarth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <osgEarth/TileSource>
#include <osgEarth/Registry>
#include <osgEarth/ImageToHeightFieldConverter>
#include <osgEarth/FileUtils>

#include <osg/Notify>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "TileCacheOptions"

#include <sstream>

using namespace osgEarth;
using namespace osgEarth::Drivers;


class TileCacheSource : public TileSource
{
public:
    TileCacheSource( const PluginOptions* options ) : TileSource( options )
    {
        _settings = dynamic_cast<const TileCacheOptions*>( options );
        if ( !_settings.valid() )
            _settings = new TileCacheOptions();
    }

    void initialize( const std::string& referenceURI, const Profile* overrideProfile)
    {
        _configPath = referenceURI;

		if (overrideProfile)
		{
		    //If we were given a profile, take it on.
			setProfile(overrideProfile);
		}
		else
		{
			//Assume it is global-geodetic
			setProfile( osgEarth::Registry::instance()->getGlobalGeodeticProfile() );
		}            
    }

    osg::Image* createImage( const TileKey* key,
                             ProgressCallback* progress)
    {
        unsigned int level, tile_x, tile_y;
        level = key->getLevelOfDetail() +1;
        key->getTileXY( tile_x, tile_y );

        unsigned int numCols, numRows;
        key->getProfile()->getNumTiles(level, numCols, numRows);
        
        // need to invert the y-tile index
        tile_y = numRows - tile_y - 1;

        char buf[2048];
        sprintf( buf, "%s/%s/%02d/%03d/%03d/%03d/%03d/%03d/%03d.%s",
            _settings->url()->c_str(),
            _settings->layer()->c_str(),
            level,
            (tile_x / 1000000),
            (tile_x / 1000) % 1000,
            (tile_x % 1000),
            (tile_y / 1000000),
            (tile_y / 1000) % 1000,
            (tile_y % 1000),
            _settings->format()->c_str() );

       
        std::string path = buf;

        //If we have a relative path and the map file contains a server address, just concat the server path and the cache together.
        if (osgEarth::isRelativePath(path) && osgDB::containsServerAddress( _configPath ) )
        {
            path = osgDB::getFilePath(_configPath) + std::string("/") + path;
        }

        //If the path doesn't contain a server address, get the full path to the file.
        if (!osgDB::containsServerAddress(path))
        {
            path = osgEarth::getFullPath(_configPath, path);
        }
        
        osg::ref_ptr<osg::Image> image;
        HTTPClient::readImageFile(path, image, getOptions(), progress );
        return image.release();

        //if (osgDB::containsServerAddress(path))
        //{
        //    //Use the HTTPClient if it's a server address.
        //    return HTTPClient::readImageFile( path, getOptions(), progress );
        //}

        //return osgDB::readImageFile( path, getOptions() );
    }

    virtual std::string getExtension()  const 
    {
        return _settings->format().value();
    }

private:
    std::string _configPath;
    osg::ref_ptr<const TileCacheOptions> _settings;
};

// Reads tiles from a TileCache disk cache.
class TileCacheSourceFactory : public osgDB::ReaderWriter
{
    public:
        TileCacheSourceFactory() {}

        virtual const char* className()
        {
            return "TileCache disk cache ReaderWriter";
        }
        
        virtual bool acceptsExtension(const std::string& extension) const
        {
            return osgDB::equalCaseInsensitive( extension, "osgearth_tilecache" );
        }

        virtual ReadResult readObject(const std::string& file_name, const Options* options) const
        {
            std::string ext = osgDB::getFileExtension( file_name );
            if ( !acceptsExtension( ext ) )
            {
                return ReadResult::FILE_NOT_HANDLED;
            }

            return new TileCacheSource( static_cast<const PluginOptions*>(options) );
        }
};

REGISTER_OSGPLUGIN(osgearth_tilecache, TileCacheSourceFactory)

