// -------------------------------------------------------------------------------- //
//	Copyright (C) 2008-2009 J.Rios
//	anonbeat@gmail.com
//
//    This Program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2, or (at your option)
//    any later version.
//
//    This Program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; see the file LICENSE.  If not, write to
//    the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//    http://www.gnu.org/copyleft/gpl.html
//
// -------------------------------------------------------------------------------- //
#include "MusicDns.h"

#include "Commands.h"
#include "curl/http.h"
#include "Utils.h"

#define guMUSICDNS_CLIENT_ID    "ca3d48c7383db1dcf6dccd1f0cab26e5"
#define guMUSICDNS_BASEURL      "http://ofa.musicdns.org/ofa/1/track"

#define guMUSICDNS_REQSTR_FP    "cid=%s&cvr=%s&fpt=%s&rmd=%d&" \
                                "brt=%d&fmt=%s&dur=%ld&art=%s&" \
                                "ttl=%s&alb=%s&tnm=%d&gnr=%s&" \
                                "yrr=%s&enc=UTF-8&\r\n"

#define guMUSICDNS_REQSTR_PUID  "cid=%s&cvr=%s&pid=%s&rmd=%d&" \
                                "brt=%d&fmt=%s&dur=%ld&art=%s&" \
                                "ttl=%s&alb=%s&tnm=%d&gnr=%s&" \
                                "yrr=%s&enc=UTF-8&\r\n"

DECLARE_EVENT_TYPE( guMUSICDNS_EVENT_FINGERPRINT, wxID_ANY )

DEFINE_EVENT_TYPE( guMUSICDNS_EVENT_FINGERPRINT )
DEFINE_EVENT_TYPE( guMUSICDNS_EVENT_PUID )
DEFINE_EVENT_TYPE( guMUSICDNS_EVENT_SEARCHDONE )


extern "C" {

//#include <gstofa.h>
// From gstofa.h
#define GST_TAG_OFA_FINGERPRINT "ofa-fingerprint"

//// -------------------------------------------------------------------------------- //
//void list_tags( const GstTagList * list, const gchar * tag, gpointer user_data )
//{
//    printf( "Tag: %s\n", tag );
//}

// -------------------------------------------------------------------------------- //
static gboolean gst_bus_async_callback( GstBus * bus, GstMessage * message, guMusicDnsThread * pobj )
{
    //guLogMessage( wxT( "Got gstreamer message %u" ), GST_MESSAGE_TYPE( message ) );
    switch( GST_MESSAGE_TYPE( message ) )
    {
        case GST_MESSAGE_ERROR :
        {
            GError * err;
            gchar * debug;
            gst_message_parse_error( message, &err, &debug );
            guLogError( wxT( "Gstreamer error '%s'" ), wxString( err->message, wxConvUTF8 ).c_str() );
            g_error_free( err );
            g_free( debug );

            pobj->Stop();
            break;
        }

        case GST_MESSAGE_STATE_CHANGED:
        {
            GstState oldstate, newstate, pendingstate;
            gst_message_parse_state_changed( message, &oldstate, &newstate, &pendingstate );
            //guLogMessage( wxT( "State changed... %u  %u  %u" ), oldstate, newstate, pendingstate );
            //wxMediaEvent event( wxEVT_MEDIA_STATECHANGED );
            //ctrl->AddPendingEvent( event );
            break;
        }

//        case GST_MESSAGE_BUFFERING :
//        {
//            guLogMessage( wxT( "Buffering..." ) );
//            break;
//        }

        case GST_MESSAGE_EOS :
        {
          //guLogMessage( wxT( "EOS Detected..." ) );
          pobj->Stop();
          break;
        }

        case GST_MESSAGE_TAG :
        {
            /* The stream discovered new tags. */
            GstTagList * tags;
            gchar * fingerprint = NULL;
            /* Extract from the message the GstTagList.
            * This generates a copy, so we must remember to free it.*/
            gst_message_parse_tag( message, &tags );

            //gst_tag_list_foreach( tags, ( GstTagForeachFunc ) list_tags, NULL );

            /* Extract the title and artist tags - if they exist */
            gst_tag_list_get_string( tags, GST_TAG_OFA_FINGERPRINT, &fingerprint );

            if( fingerprint )
            {
                //guLogMessage( wxT( "Gstreamer got fingerprint '%s'" ), wxString( fingerprint, wxConvUTF8 ).c_str() );
                pobj->SetFingerprint( fingerprint );
                pobj->Stop();
            }

            /* Free the tag list */
            gst_tag_list_free( tags );
            break;
        }

        default:
            break;
    }

    return TRUE;
}

// -------------------------------------------------------------------------------- //
static void on_pad_added( GstElement * decodebin, GstPad * pad, gboolean last, GstElement * conv )
{
  GstCaps * caps;
  GstStructure * str;
  GstPad * convpad;
//  GstElement * conv;
//  conv = ( GstElemet * ) data;

//  guLogMessage( wxT( "New pad created..." ) );

  convpad = gst_element_get_pad( conv, "sink" );

  /* check media type */
  caps = gst_pad_get_caps( pad );
  str = gst_caps_get_structure( caps, 0 );
  if( !g_strrstr( gst_structure_get_name( str ), "audio" ) )
  {
    gst_caps_unref( caps );
    gst_object_unref( convpad );
    return;
  }
  gst_caps_unref( caps );

//  guLogMessage( wxT( "Linked decoder and converter..." ) );
  /* link'n'play */
  gst_pad_link( pad, convpad );
}


}

// -------------------------------------------------------------------------------- //
// guMusicDnsThread
// -------------------------------------------------------------------------------- //
guMusicDnsThread::guMusicDnsThread( guMusicDns * musicdns, const wxChar * filename )
{
    wxASSERT( musicdns );

    guLogMessage( wxT( "guMusicDnsThread..." ) );
    m_MusicDns = musicdns;
    m_FileName = wxString( filename );
    m_Running = false;

    m_Pipeline = gst_pipeline_new( "guPipeline" );
    if( !GST_IS_ELEMENT( m_Pipeline ) )
        guLogError( wxT( "Error creating the MusicDns pipeline" ) );

    GstElement * src;
    src = gst_element_factory_make( "filesrc", "guSource" );
    g_object_set( G_OBJECT( src ), "location", ( const char * ) m_FileName.mb_str(), NULL );
    if( !GST_IS_ELEMENT( src ) )
        guLogError( wxT( "Error creating the MusicDns source" ) );

    GstElement * dec;
    dec = gst_element_factory_make( "decodebin", "guDecoder" );
    if( !GST_IS_ELEMENT( dec ) )
        guLogError( wxT( "Error creating the MusicDns decoder" ) );

    GstElement * conv;
    conv = gst_element_factory_make( "audioconvert", "guConverter" );
    if( !GST_IS_ELEMENT( conv ) )
        guLogError( wxT( "Error creating the MusicDns converter" ) );

    GstElement * ofa;
    ofa = gst_element_factory_make( "ofa", "guOFA" );
    if( !GST_IS_ELEMENT( ofa ) )
        guLogError( wxT( "Error creating the MusicDns ofa" ) );

    GstElement * fake;
    fake = gst_element_factory_make( "fakesink", "guFakeSink" );
    g_object_set( G_OBJECT( fake ), "sync", 0, NULL );
    if( !GST_IS_ELEMENT( fake ) )
        guLogError( wxT( "Error creating the MusicDns fakeout" ) );

    gst_bin_add_many( GST_BIN( m_Pipeline ), src, dec, conv, ofa, fake, NULL );

    GstBus * bus = gst_pipeline_get_bus( GST_PIPELINE( m_Pipeline ) );
    gst_bus_add_watch( bus, ( GstBusFunc ) gst_bus_async_callback, this );
    gst_object_unref( G_OBJECT( bus ) );


    if( !gst_element_link( src, dec ) )
        guLogError( wxT( "Error linking the objects src, dec" ) );

    g_signal_connect( dec, "new-decoded-pad", G_CALLBACK( on_pad_added ), conv );

    if( !gst_element_link_many( conv, ofa, fake, NULL ) )
        guLogError( wxT( "Error linking the objects conv, ofa, fake" ) );

    gst_element_set_state( m_Pipeline, GST_STATE_PAUSED );

    guLogMessage( wxT( "Created the pipeline..." ) );

    if( Create() == wxTHREAD_NO_ERROR )
    {
        SetPriority( WXTHREAD_DEFAULT_PRIORITY - 30 );
        Run();
    }
}

// -------------------------------------------------------------------------------- //
guMusicDnsThread::~guMusicDnsThread()
{
    wxASSERT( GST_IS_OBJECT( m_Pipeline ) );
    if( m_Pipeline )
    {
        gst_element_set_state( m_Pipeline, GST_STATE_NULL );
        gst_object_unref( GST_OBJECT( m_Pipeline ) );
    }
    m_MusicDns->ClearMusicDnsThread();
    guLogMessage( wxT( "Destroyed MusicDnsThread..." ) );
}

// -------------------------------------------------------------------------------- //
guMusicDnsThread::ExitCode guMusicDnsThread::Entry()
{
    gst_element_set_state( m_Pipeline, GST_STATE_PLAYING );

    m_Running = true;

    while( !TestDestroy() && m_Running )
    {
        Sleep( 10 );
    }
    guLogMessage( wxT( "Finished guMusicDnsThread..." ) );
}

// -------------------------------------------------------------------------------- //
void guMusicDnsThread::SetFingerprint( const char * fingerprint )
{
    wxCommandEvent event( guMUSICDNS_EVENT_FINGERPRINT );
    event.SetClientData( new wxString( fingerprint, wxConvUTF8 ) );
    m_MusicDns->AddPendingEvent( event );
}

// -------------------------------------------------------------------------------- //
void guMusicDnsThread::Stop( void )
{
    m_Running = false;
}

// -------------------------------------------------------------------------------- //
// guMusicDns
// -------------------------------------------------------------------------------- //
guMusicDns::guMusicDns( void )
{
    m_MusicDnsThread = NULL;

    Connect( guMUSICDNS_EVENT_FINGERPRINT, wxCommandEventHandler( guMusicDns::OnFingerprint ), NULL, this );
}

// -------------------------------------------------------------------------------- //
guMusicDns::~guMusicDns()
{
    if( m_MusicDnsThread )
    {
        m_MusicDnsThread->Pause();
        m_MusicDnsThread->Delete();
    }
}

// -------------------------------------------------------------------------------- //
void guMusicDns::SetTrack( guTrack * track )
{
    m_Track = track;
    m_Fingerprint = wxT( "" );
    m_PUID = wxT( "" );
    DoGetFingerprint();
}

// -------------------------------------------------------------------------------- //
wxString guMusicDns::GetXmlDoc( void )
{
    return m_XmlDoc;
}

// -------------------------------------------------------------------------------- //
wxString guMusicDns::GetFingerprint( void )
{
    return m_Fingerprint;
}

// -------------------------------------------------------------------------------- //
void guMusicDns::OnFingerprint( wxCommandEvent &event )
{
    wxString * fingerprint = ( wxString * ) event.GetClientData();
    guLogMessage( wxT( "OnFingerprint %s" ), fingerprint->c_str() );
    SetFingerprint( * fingerprint );
    delete fingerprint;
}

// -------------------------------------------------------------------------------- //
void guMusicDns::SetFingerprint( const wxString &fingerprint )
{
    m_Fingerprint = fingerprint;
    if( !m_Fingerprint.IsEmpty() )
        DoGetMetadata();
}

// -------------------------------------------------------------------------------- //
void guMusicDns::SetFingerprint( const char * fingerprint )
{
    SetFingerprint( wxString( fingerprint, wxConvUTF8 ) );
}

// -------------------------------------------------------------------------------- //
void guMusicDns::SetXmlDoc( const wxString &xmldoc )
{
    int EndPos = xmldoc.Find( wxT( "</metadata>" ) ) + 11;
    m_XmlDoc = xmldoc.Mid( 0, EndPos );
    guLogMessage( wxT( "XmlDoc:\n%s" ), m_XmlDoc.c_str() );
    if( !m_XmlDoc.IsEmpty() )
        DoParseXmlDoc();
}

// -------------------------------------------------------------------------------- //
wxString guMusicDns::GetPUID( void )
{
    return m_PUID;
}

// -------------------------------------------------------------------------------- //
void guMusicDns::SetPUID( const wxString &puid )
{
    m_PUID = puid;
    //
    wxCommandEvent event( guMUSICDNS_EVENT_PUID );
    AddPendingEvent( event );
}

// -------------------------------------------------------------------------------- //
void guMusicDns::SetPUID( const char * puid )
{
    SetPUID( wxString( puid, wxConvUTF8 ) );
}

// -------------------------------------------------------------------------------- //
void guMusicDns::ClearMusicDnsThread( void )
{
    m_MusicDnsThread = NULL;
    wxCommandEvent event( guMUSICDNS_EVENT_SEARCHDONE );
    AddPendingEvent( event );
}

// -------------------------------------------------------------------------------- //
bool guMusicDns::IsRunning( void )
{
    return m_MusicDnsThread != NULL;
}

void guMusicDns::CancelSearch( void )
{
    if( m_MusicDnsThread )
    {
        m_MusicDnsThread->Pause();
        m_MusicDnsThread->Delete();
    }
}

// -------------------------------------------------------------------------------- //
bool guMusicDns::DoGetFingerprint( void )
{
    wxASSERT( m_Track );

    guLogMessage( wxT( "DoGetFingerprint..." ) );
    if( m_Track )
    {
        m_MusicDnsThread = new guMusicDnsThread( this, m_Track->m_FileName.c_str() );
    }
    return m_MusicDnsThread != NULL;
}

// -------------------------------------------------------------------------------- //
bool guMusicDns::DoGetMetadata( void )
{
    wxString HtmlData = wxString::Format( wxT( guMUSICDNS_REQSTR_FP ),
        wxT( guMUSICDNS_CLIENT_ID ),
        wxT( ID_GUAYADEQUE_VERSION ),
        m_Fingerprint.c_str(),
        0, //1,
        m_Track->m_Bitrate,
        m_Track->m_FileName.AfterLast( wxT( '.' ) ).c_str(),
        m_Track->m_Length * 1000,
        !m_Track->m_ArtistName.IsEmpty() ? m_Track->m_ArtistName.c_str() : wxT( "unknown" ),
        !m_Track->m_SongName.IsEmpty() ? m_Track->m_SongName.c_str() : wxT( "unknown" ),
        !m_Track->m_AlbumName.IsEmpty() ? m_Track->m_AlbumName.c_str() : wxT( "unknown" ),
        m_Track->m_Number,
        !m_Track->m_GenreName.IsEmpty() ? m_Track->m_GenreName.c_str() : wxT( "unknown" ),
        wxString::Format( wxT( "%u" ), m_Track->m_Year ).c_str() );


    guLogMessage( wxT( guMUSICDNS_BASEURL ) wxT( "%s" ), HtmlData.c_str() );
    wxCurlHTTP  http;
    http.AddHeader( wxT( "User-Agent: Mozilla/5.0 (X11; U; Linux i686; es-ES; rv:1.9.0.5) Gecko/2008121622 Ubuntu/8.10 (intrepid) Firefox/3.0.5" ) );
    http.AddHeader( wxT( "Accept: text/html" ) );
    http.AddHeader( wxT( "Accept-Charset: utf-8" ) );
    if( http.Post( wxCURL_STRING2BUF( HtmlData ), HtmlData.Length(), wxT( guMUSICDNS_BASEURL ) ) )
    {
        SetXmlDoc( http.GetResponseBody() );
    }
}

// -------------------------------------------------------------------------------- //
bool guMusicDns::ReadTrackInfo( wxXmlNode * XmlNode )
{
//    <?xml version="1.0" encoding="UTF-8"?>
//    <metadata xmlns="http://musicbrainz.org/ns/mmd-1.0#" xmlns:creativeCommons="http://backend.userland.com/creativeCommonsRssModule" xmlns:mip="http://musicip.com/ns/mip-1.0#">
//      <track>
//        <title>Song Title</title>
//        <artist>
//          <name>Artist Name</name>
//        </artist>
//        <puid-list>
//          <puid id="2c43ec65-b629-f449-3f5e-c0b69527d771"/>
//        </puid-list>
//      </track>
//    </metadata>
    if( XmlNode->GetName() == wxT( "track" ) )
    {
        XmlNode = XmlNode->GetChildren();
        while( XmlNode )
        {
            if( XmlNode->GetName() == wxT( "puid-list" ) )
            {
                XmlNode = XmlNode->GetChildren();
                continue;
            }
            else if( XmlNode->GetName() == wxT( "puid" ) )
            {
                wxString PUId;
                XmlNode->GetPropVal( wxT( "id" ), &PUId );
                SetPUID( PUId );
                return true;
            }
            XmlNode = XmlNode->GetNext();
        }
    }
    return false;
}

// -------------------------------------------------------------------------------- //
bool guMusicDns::DoParseXmlDoc( void )
{
    wxStringInputStream ins( m_XmlDoc );
    wxXmlDocument XmlDoc( ins );
    wxXmlNode * XmlNode = XmlDoc.GetRoot();
    if( XmlNode && ( XmlNode->GetName() == wxT( "metadata" ) ) )
    {
        ReadTrackInfo( XmlNode->GetChildren() );
    }
}

// -------------------------------------------------------------------------------- //