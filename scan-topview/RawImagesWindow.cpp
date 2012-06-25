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


#include "RawImagesWindow.h"
#include "ui_RawImagesWindow.h"

#include "GuiController.h"
#include "ModelAcquisitionController.h"

#include <ntk/camera/rgbd_frame_recorder.h>
#include <ntk/camera/rgbd_processor.h>
#include <ntk/utils/opencv_utils.h>
#ifdef NESTK_USE_FREENECT
# include <ntk/camera/freenect_grabber.h>
#endif

#include <QCloseEvent>
#include <QFileDialog>

using namespace ntk;

RawImagesWindow::RawImagesWindow(GuiController& controller, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RawImagesWindow),
    m_controller(controller)
{
    ui->setupUi(this);

    connect(ui->depthView, SIGNAL(mouseMoved(int,int)),
            &m_controller, SLOT(on_depth_mouse_moved(int,int)));
    ui->depthView->setMouseTracking(true);

    ui->integrationTimeText->setText(QString("%1").arg(
                                         m_controller.grabber().integrationTime()));

    ui->action_Show_Object_Detector->setDisabled(true);
    ui->action_3D_View->setDisabled(true);

    if (m_controller.grabber().isSynchronous())
        ui->syncMode->setChecked(true);
}

RawImagesWindow::~RawImagesWindow()
{
    delete ui;
}

void RawImagesWindow :: update(const ntk::RGBDImage& image)
{
    if (ui->colorView->isVisible())
        ui->colorView->setImage(image.rgb());
    if (ui->depthView->isVisible())
    {
        double min_dist = m_controller.rgbdProcessor().minDepth();
        double max_dist = m_controller.rgbdProcessor().maxDepth();
        cv::Mat1f masked_distance; image.depth().copyTo(masked_distance);
        apply_mask(masked_distance, image.depthMask());
        cv::Mat3b depth_as_color;
        compute_color_encoded_depth(masked_distance, depth_as_color, &min_dist, &max_dist);
        ui->depthView->setImage(depth_as_color);
    }
    // ui->depthView->setImageAsColor(image.depth(), &min_dist, &max_dist);
    // ui->depthView->setImage(image.depth(), &min_dist, &max_dist);
    if (image.amplitude().data && ui->amplitudeView->isVisible())
        ui->amplitudeView->setImage(image.amplitude());
    if (image.intensity().data && ui->intensityView->isVisible())
        ui->intensityView->setImage(image.intensity());

    int x,y;
    ui->depthView->getLastMousePos(x,y);
    m_controller.on_depth_mouse_moved(x,y);
}

void RawImagesWindow::on_outputDirText_editingFinished()
{
    QString dir = ui->outputDirText->text();
    m_controller.frameRecorder()->setDirectory(dir.toStdString());
}

void RawImagesWindow::on_action_GrabOneFrame_triggered()
{
    m_controller.saveCurrentFrame();
}

void RawImagesWindow::on_action_Quit_triggered()
{
    m_controller.quit();
}

void RawImagesWindow::on_action_3D_View_toggled(bool active)
{
    m_controller.toggleView3d(active);
}

void RawImagesWindow::on_action_Filters_toggled(bool active)
{
    m_controller.toggleFilters(active);
}

void RawImagesWindow::on_action_Screen_capture_mode_toggled(bool active)
{
    m_controller.setScreenCaptureMode(active);
}

void RawImagesWindow::on_action_GrabFrames_toggled(bool active)
{
    m_controller.setGrabFrames(active);
}

void RawImagesWindow::on_syncMode_toggled(bool checked)
{
    m_controller.grabber().setSynchronous(checked);
    if (checked)
        m_controller.grabber().newEvent();
}

void RawImagesWindow::on_action_Show_Modeler_toggled(bool active)
{
    m_controller.toggleModeler(active);
}

void RawImagesWindow::closeEvent(QCloseEvent *event)
{
    ui->action_Quit->trigger();
    event->accept();
}

void RawImagesWindow::on_actionPause_toggled(bool active)
{
    // m_controller.rgbdProcessor().setFilterFlag(RGBDProcessorFlags::Pause, active);
    m_controller.setPaused(active);
}

void RawImagesWindow::on_actionNext_frame_triggered()
{
    m_controller.processOneFrame();
}

void RawImagesWindow::on_integrationTimeText_returnPressed()
{
    int value = ui->integrationTimeText->text().toInt();
    m_controller.grabber().setIntegrationTime(value);
}

void RawImagesWindow::on_actionShow_IR_toggled(bool v)
{
#ifdef NESTK_USE_FREENECT
    KinectGrabber* kinect_grabber = dynamic_cast<KinectGrabber*>(&m_controller.grabber());
    if (kinect_grabber)
        kinect_grabber->setIRMode(v);
#endif
}

void RawImagesWindow::on_actionDual_RGB_IR_mode_toggled(bool v)
{
#ifdef NESTK_USE_FREENECT
    KinectGrabber* kinect_grabber = dynamic_cast<KinectGrabber*>(&m_controller.grabber());
    if (kinect_grabber)
        kinect_grabber->setDualRgbIR(v);
#endif
}

void RawImagesWindow::on_actionSave_calibration_parameters_triggered()
{
    if (!m_controller.lastImage().calibration())
    {
        ntk_dbg(0) << "Images do not have calibration.";
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Save calibration as...",
                                                    QString("calibration.yml"));
    if (filename.isEmpty())
        return;
    m_controller.lastImage().calibration()->saveToFile(filename.toAscii());
}
