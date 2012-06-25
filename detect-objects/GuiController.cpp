/**
 *
 * This file is a part of the MetaLAB_rgbdemo software
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "GuiController.h"
#include "ui_View3dWindow.h"
#include "ui_RawImagesWindow.h"
#include "View3dWindow.h"
#include "FiltersWindow.h"
#include "RawImagesWindow.h"
#include "ObjectWindow.h"

#include <ntk/ntk.h>
#include <ntk/camera/rgbd_frame_recorder.h>
#include <ntk/geometry/relative_pose_estimator_icp.h>
#include <ntk/mesh/mesh_generator.h>
#include <ntk/mesh/mesh_viewer.h>
#include <ntk/mesh/mesh_renderer.h>
#include <ntk/detection/plane_estimator.h>

#include <QMainWindow>
#include <QImage>
#include <QApplication>

#include <fstream>

using namespace cv;
using namespace ntk;

GuiController :: GuiController(ntk::RGBDGrabber& producer,
                               ntk::RGBDProcessor& processor)
    : m_grabber(producer),
      m_processor(processor),
      m_frame_recorder(0),
      m_mesh_generator(0),
      m_plane_estimator(0),
      m_last_tick(cv::getTickCount()),
      m_frame_counter(0),
      m_frame_rate(0),
      m_view3d_window_grabber("/tmp/demo3d/view3d"),
      m_raw_window_grabber("/tmp/demo3d/raw"),
      m_screen_capture_mode(false),
      m_grab_frames(false),
      m_paused(false),
      m_process_one_frame(false),
      m_show_plane(false),
      m_new_model(false)
{
    m_raw_images_window = new RawImagesWindow(*this);
    m_view3d_window = new View3DWindow(*this);
    m_filters_window = new FiltersWindow(*this);
    m_object_window = new ObjectWindow(*this);

    m_raw_images_window->show();
    m_object_window->show();
}

GuiController :: ~GuiController()
{
    delete m_raw_images_window;
    delete m_view3d_window;
    delete m_filters_window;
    delete m_object_window;
}

void GuiController :: quit()
{
    m_grabber.setShouldExit();
    m_grabber.newEvent();
    m_grabber.wait();
    QApplication::quit();
}

void GuiController :: setDatabasePath(const std::string& path)
{
    m_object_window->setDatabasePath(path);
}

void GuiController :: setFrameRecorder(ntk::RGBDFrameRecorder& frame_recorder)
{
    m_frame_recorder = &frame_recorder;
    m_raw_images_window->ui->outputDirText->setText(m_frame_recorder->directory().path());
}

void GuiController :: setMeshGenerator(MeshGenerator& generator)
{
    m_mesh_generator = &generator;
    m_raw_images_window->ui->action_3D_View->setEnabled(true);
}

static QImage toNormalizedQImage(const cv::Mat1f& m)
{
    cv::Mat1b norm;
    cv::normalize(m, norm, 0, 255, NORM_MINMAX, 0);

    QImage qim (norm.cols, norm.rows, QImage::Format_RGB32);
    for (int r = 0; r < norm.rows; ++r)
        for (int c = 0; c < norm.cols; ++c)
        {
            int v = norm(r,c);
            qim.setPixel(c,r, qRgb(v,v,v));
        }
    return qim;
}

static QImage toQImage(const cv::Mat3b& m)
{
    QImage qim (m.cols, m.rows, QImage::Format_RGB32);
    for (int r = 0; r < m.rows; ++r)
    {
        QRgb* ptr = (QRgb*) qim.scanLine(r);
        for (int c = 0; c < m.cols; ++c)
        {
            const Vec3b& v = m(r,c);
            *ptr = qRgb(v[2],v[1],v[0]);
            ++ptr;
        }
    }
    return qim;
}

void GuiController :: saveCurrentFrame()
{
    std::string frame_dir = m_frame_recorder->getNextFrameDirectory(lastImage());
    m_frame_recorder->saveCurrentFrame(lastImage());
}

void GuiController :: onRGBDDataUpdated()
{
    if (m_paused && !m_process_one_frame)
        return;

    m_process_one_frame = false;

    ++m_frame_counter;
    if (m_frame_counter == 10)
    {
        double current_tick = cv::getTickCount();
        m_frame_rate = m_frame_counter / ((current_tick - m_last_tick)/cv::getTickFrequency());
        m_last_tick = current_tick;
        m_frame_counter = 0;
    }

    m_grabber.copyImageTo(m_last_image);
    TimeCount tc_rgbd_process("m_processor.processImage", 2);
    m_processor.processImage(m_last_image);
    tc_rgbd_process.stop();

    if (m_raw_images_window->isVisible())
        m_raw_images_window->update(m_last_image);

    if (m_frame_recorder && (m_screen_capture_mode || m_grab_frames))
    {
        saveCurrentFrame();
    }

    if (m_screen_capture_mode)
        m_raw_window_grabber.saveFrame(QPixmap::grabWindow(m_raw_images_window->winId()));

    if (m_plane_estimator && m_show_plane)
    {
        cv::Mat1b m_plane_image (m_last_image.amplitude().cols,m_last_image.amplitude().rows);
        m_plane_estimator->estimate(m_last_image, m_plane_image);
        ntk::Mesh plane_mesh;
        generate_mesh_from_plane(plane_mesh, m_plane_estimator->currentPlane(), Point3f(0,0,-1), 1);
        m_view3d_window->ui->mesh_view->addMesh(plane_mesh, Pose3D(), MeshViewer::FLAT);
    }

    if (m_mesh_generator && m_view3d_window->isVisible())
    {
        TimeCount tc_generate ("Mesh generate");
        m_mesh_generator->generate(m_last_image);
        tc_generate.stop();

        TimeCount tc_add_mesh ("Add mesh");
        m_view3d_window->ui->mesh_view->addMesh(m_mesh_generator->mesh(), Pose3D(), MeshViewer::FLAT);
        tc_add_mesh.stop();

        if (m_screen_capture_mode)
            m_view3d_window_grabber.saveFrame(QPixmap::grabWindow(m_view3d_window->winId()));
        m_view3d_window->ui->mesh_view->swapScene();
    }

    QString status = QString("Final fps = %1 fps GRABBER = %2")
            .arg(m_frame_rate, 0, 'f', 1)
            .arg(m_grabber.frameRate(), 0, 'f', 1);
    if (m_grab_frames)
        status += " [GRABBING]";
    m_raw_images_window->ui->statusbar->showMessage(status);

    if (!m_paused)
        m_grabber.newEvent();
}

void GuiController :: on_depth_mouse_moved(int x, int y)
{
    if (!m_last_image.depth().data || x < 0 || y < 0)
        return;
    QString s = QString("Distance at (%1,%2) = %3 m")
            .arg(x).arg(y).arg(m_last_image.depth()(y,x), 0, 'f', 3);
    m_raw_images_window->ui->distanceLabel->setText(s);
}

void GuiController::toggleView3d(bool active)
{
    if (active)
    {
        m_view3d_window->show();
    }
    else
    {
        m_view3d_window->hide();
    }
}

void GuiController::toggleFilters(bool active)
{
    if (active)
    {
        m_filters_window->show();
    }
    else
    {
        m_filters_window->hide();
    }
}

void GuiController::toggleObject(bool active)
{
    if (active)
    {
        m_object_window->show();
    }
    else
    {
        m_object_window->hide();
    }
}
