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

#include "FiltersWindow.h"
#include "ui_FiltersWindow.h"

#include "GuiController.h"
#include <ntk/camera/rgbd_grabber.h>
#include <ntk/camera/rgbd_processor.h>

using namespace ntk;

FiltersWindow::FiltersWindow(GuiController& controller, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FiltersWindow),
    m_controller(controller)
{
  ui->setupUi(this);

  if ( m_controller.rgbdProcessor().hasFilterFlag(RGBDProcessorFlags::FilterEdges))
    ui->edgesCheckBox->setCheckState(Qt::Checked);

  if ( m_controller.rgbdProcessor().hasFilterFlag(RGBDProcessorFlags::FilterThresholdDepth))
    ui->depthThresholdCheckBox->setCheckState(Qt::Checked);

  if ( m_controller.rgbdProcessor().hasFilterFlag(RGBDProcessorFlags::FilterNormals))
    ui->normalsCheckBox->setCheckState(Qt::Checked);

  if ( m_controller.rgbdProcessor().hasFilterFlag(RGBDProcessorFlags::FilterUnstable))
    ui->unstableCheckBox->setCheckState(Qt::Checked);

  if ( m_controller.rgbdProcessor().hasFilterFlag(RGBDProcessorFlags::FilterMedian))
    ui->medianCheckBox->setCheckState(Qt::Checked);

  ui->minDepthLabel->setText(QString("%1 m").arg(m_controller.rgbdProcessor().minDepth(), 0, 'f', 2));
  ui->maxDepthLabel->setText(QString("%1 m").arg(m_controller.rgbdProcessor().maxDepth(), 0, 'f', 2));
  updateDepthSlider(); // to update labels.
}

FiltersWindow::~FiltersWindow()
{
  delete ui;
}

void FiltersWindow::on_depthThresholdCheckBox_toggled(bool checked)
{
  m_controller.rgbdProcessor().setFilterFlag(RGBDProcessorFlags::FilterThresholdDepth, checked);
}

void FiltersWindow::on_edgesCheckBox_toggled(bool checked)
{
  m_controller.rgbdProcessor().setFilterFlag(RGBDProcessorFlags::FilterEdges, checked);
}

void FiltersWindow::on_medianCheckBox_toggled(bool checked)
{
  m_controller.rgbdProcessor().setFilterFlag(RGBDProcessorFlags::FilterMedian, checked);
}

void FiltersWindow::on_normalsCheckBox_toggled(bool checked)
{
  m_controller.rgbdProcessor().setFilterFlag(RGBDProcessorFlags::FilterNormals, checked);
  m_controller.rgbdProcessor().setMaxNormalAngle(60);
}

void FiltersWindow::on_unstableCheckBox_toggled(bool checked)
{
  m_controller.rgbdProcessor().setFilterFlag(RGBDProcessorFlags::FilterNormals, checked);
}

void FiltersWindow::updateDepthSlider()
{
  double min_meters = ui->minDepthSlider->value()/100.0;
  double max_meters = ui->maxDepthSlider->value()/100.0;

  ui->minDepthLabel->setText(QString("%1 m").arg(min_meters, 0, 'f', 2));
  ui->maxDepthLabel->setText(QString("%1 m").arg(max_meters, 0, 'f', 2));

  m_controller.rgbdProcessor().setMinDepth(min_meters);
  m_controller.rgbdProcessor().setMaxDepth(max_meters);
}

void FiltersWindow::on_minDepthSlider_valueChanged(int value)
{
  updateDepthSlider();
}

void FiltersWindow::on_maxDepthSlider_valueChanged(int value)
{
  updateDepthSlider();
}

void FiltersWindow::on_fillSmallHolesCheckBox_toggled(bool checked)
{
  m_controller.rgbdProcessor().setFilterFlag(RGBDProcessorFlags::FillSmallHoles, checked);
}

void FiltersWindow::on_removeSmallStructuresBox_toggled(bool checked)
{
  m_controller.rgbdProcessor().setFilterFlag(RGBDProcessorFlags::RemoveSmallStructures, checked);
}

void FiltersWindow::on_kinectTiltSlider_valueChanged(int value)
{
  m_controller.grabber().setTiltAngle(value);
}
