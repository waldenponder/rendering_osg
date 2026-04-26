#include "ThreeDimManipulator.h"
#include <osg/ComputeBoundsVisitor>
#include <osgUtil/IntersectionVisitor>
#include <random>

ThreeDimManipulator::ThreeDimManipulator(osgViewer::Viewer* viewer) : _viewer(viewer)
{
	setAllowThrow(false);
}

ThreeDimManipulator::~ThreeDimManipulator()
{
}

bool ThreeDimManipulator::handleMouseDrag(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
	//LOG_INFO << "DRAG\t" << ea.getButton() << "\n";
	//用ea.getButton()获取的不是osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON
	if (_buttonType != osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON) return false;

	//按住shift,执行旋转操作，
	if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)
	{
		if (_ga_t1 && _ga_t0)
			rotateTrackball(_ga_t0->getXnormalized(), _ga_t0->getYnormalized(),
			                _ga_t1->getXnormalized(), _ga_t1->getYnormalized(),
			                getThrowScale(0));
	}
	else
	{
		osg::Camera* camera = getCamera();

		if (camera)
		{
			//--------------------------------------------------先求交，如果有交点，则要满足拖拽时鼠标跟随
			if (_ga_t0 && _ga_t1)
			{
				osg::ref_ptr<osgUtil::IntersectorGroup> intersectors = new osgUtil::IntersectorGroup;

				intersectors->addIntersector(new osgUtil::LineSegmentIntersector(
					osgUtil::Intersector::PROJECTION, _ga_t0->getXnormalized(), _ga_t0->getYnormalized()));
				osgUtil::IntersectionVisitor iv(intersectors.get());
				//	iv.setTraversalMask(~(g::NM_GRID | g::NM_BACK_GROUND));
				camera->accept(iv);

				for (osg::ref_ptr<osgUtil::Intersector>& is : intersectors->getIntersectors())
				{
					osgUtil::LineSegmentIntersector* lis = dynamic_cast<osgUtil::LineSegmentIntersector*>(is.get());
					if (lis && lis->containsIntersections())
					{
						osg::Vec3 pos = lis->getFirstIntersection().getWorldIntersectPoint();

						osg::Matrix view_m;

						osg::Vec3 pt(_ga_t0->getXnormalized(), _ga_t0->getYnormalized(), 0);

						/*				pt = pos * view_m * camera->getProjectionMatrix();
						
						pt * osg::Matrix::inverse(camera->getProjectionMatrix()) = pos * view_m;*/
						//pos * view_m = pt * osg::Matrix::inverse(camera->getProjectionMatrix());

						osg::Vec3 center;
						//view_m = osg::Matrix::translate(-center) *
						//	osg::Matrix::rotate(camera->getViewMatrix().getRotate()) *
						//	osg::Matrix::translate(0, 0, -_distance);

						osg::Vec3 PT = pt * osg::Matrix::inverse(camera->getProjectionMatrix()) *
							osg::Matrix::inverse(osg::Matrix::translate(0, 0, -_distance)) * osg::Matrix::inverse(
								osg::Matrix::rotate(camera->getViewMatrix().getRotate()));

						//pos * osg::Matrix::translate(-center) =
						//	pos - x = PT.x();
						_center = osg::Vec3(pos.x() - PT.x(), pos.y() - PT.y(), pos.z() - PT.z());

						return __super::handleMouseDrag(ea, us);
					}
				}
			}

			//--------------------------------------------------执行原来的逻辑
			osg::Vec2 deltaPt = osg::Vec2(ea.getX(), ea.getY()) - _preMousePt;
			_preMousePt = osg::Vec2(ea.getX(), ea.getY());

			float w = camera->getViewport()->width();
			float h = camera->getViewport()->height();

			double screenWidthInWorld = 1000; // _viewer->screenWidthInWorld();
			osg::Vec3d dv = osg::Vec3(deltaPt[0] / w * screenWidthInWorld,
			                          deltaPt[1] / h * (screenWidthInWorld * (h / w)), 0);

			osg::Matrix rotation_matrix;
			rotation_matrix.makeRotate(_rotation);

			_center -= dv * rotation_matrix;
		}
	}

	return __super::handleMouseDrag(ea, us);
}

bool ThreeDimManipulator::handleKeyDown(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
	_keyType = osgGA::GUIEventAdapter::KEY_Space;

	if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Space)
	{
		return true;
	}

	return __super::handleKeyDown(ea, us);
}

bool ThreeDimManipulator::handleKeyUp(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
	_keyType = -1;
	return __super::handleKeyUp(ea, us);
}

bool ThreeDimManipulator::handleMouseWheel(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
	bool bHandle = false;

	const bool bOrth = false;
	float factor = bOrth ? .1 : .3;

	if (ea.getEventType() == osgGA::GUIEventAdapter::SCROLL)
	{
		osg::Vec3d eye, center, up;
		this->getTransformation(eye, center, up);

		//osg::Vec3 mousePt = util::screenToWorld(getCamera(), osg::Vec3(ea.getX(), ea.getY(), 0));

		//osg::Vec3 delta = bOrth ? mousePt - _center : mousePt - eye;

		//if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP)
		//{
		//	bOrth ? _zoomFactor *= (1.0 - factor) : _distance *= (1.0 - factor);
		//	_center += (delta * factor);
		//	bHandle = true;
		//}
		//else if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_DOWN)
		//{
		//	bOrth ? _zoomFactor *= (1.0 + factor) : _distance *= (1.0 + factor);
		//	_center -= (delta * factor);
		//	bHandle = true;
		//}
	}

	if (bHandle && bOrth)
	{
		const double viewPortW = _viewer->getCamera()->getViewport()->width();
		const double viewPortH = _viewer->getCamera()->getViewport()->height();

		float w = viewPortW * _zoomFactor;
		float h = viewPortH * _zoomFactor;

		_viewer->getCamera()->setProjectionMatrixAsOrtho2D(-w / 2, w / 2, -h / 2, h / 2);
	}

	return __super::handleMouseWheel(ea, us);
}

bool ThreeDimManipulator::handleMouseRelease(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
	_buttonType = -1;
	//LOG_INFO << "RELEASE\n";
	return __super::handleMouseRelease(ea, us);
}

bool ThreeDimManipulator::handleMousePush(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
	//LOG_INFO << "PUSH\n";
	_buttonType = ea.getButton();
	_preMousePt = osg::Vec2(ea.getX(), ea.getY());

	return __super::handleMousePush(ea, us);
}

void ThreeDimManipulator::rotateTrackball(const float px0, const float py0, const float px1, const float py1,
                                          const float scale)
{
#pragma region 基类的实现
	osg::Vec3d axis;
	float angle;

	trackball(axis, angle, px0 + (px1 - px0) * scale, py0 + (py1 - py0) * scale, px0, py0);

	osg::Quat new_rotate;
	new_rotate.makeRotate(angle, axis);

	_rotation = _rotation * new_rotate;
#pragma endregion

	osg::Vec3 bbCenter; //获取旋转中心
#if 0
	{
		osg::ComputeBoundsVisitor cbVisitor;

		osg::Node* centerNode = nullptr;
		list<dtData_ptr> datas = db_data->get_all_select();
		if (datas.size())
		{
			dtObject* obj = datas.front()->as_dtObject();
			if (obj && obj->object())
			{
				centerNode = dynamic_cast<osg::Node*>(obj->object());
			}
		}
		if (!centerNode)
			centerNode = _viewer->graphNode();

		centerNode->accept(cbVisitor);
		auto bb = cbVisitor.getBoundingBox();

		bbCenter = bb.valid() ? bb.center() : centerNode->getBound().center();
	}
#endif

	//以bbCenter为中心旋转_center
	_center = new_rotate * (_center - bbCenter) + bbCenter;
}

//屏蔽父类实现
bool ThreeDimManipulator::performMovement()
{
	return __super::performMovement();
	return false;
}

osg::Vec3 ThreeDimManipulator::get_word_pos(float xNormal, float yNormal)
{
	osg::ref_ptr<osgUtil::IntersectorGroup> intersectors = new osgUtil::IntersectorGroup;

	intersectors->addIntersector(
		new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, xNormal, yNormal));
	osgUtil::IntersectionVisitor iv(intersectors.get());
	_viewer->getCamera()->accept(iv);

	for (osg::ref_ptr<osgUtil::Intersector>& is : intersectors->getIntersectors())
	{
		osgUtil::LineSegmentIntersector* lis = dynamic_cast<osgUtil::LineSegmentIntersector*>(is.get());
		if (lis && lis->containsIntersections())
		{
			osg::Vec3 pos = lis->getFirstIntersection().getWorldIntersectPoint();
			return pos;
		}
	}

	return osg::Vec3();
}

osg::Camera* ThreeDimManipulator::getCamera() const
{
	osg::Camera* c = nullptr;
	if (_viewer && (c = _viewer->getCamera()))
		return c;

	return nullptr;
}

void ThreeDimManipulator::pan(int dx0, int dy0, const osg::Vec2& oldPos, const osg::Vec2& newPos)
{
	if (oldPos == newPos)
		return;
#if 0
	VKPerspectiveCameraInfo* persInfo = dynamic_cast<VKPerspectiveCameraInfo*>(m_cameraInfo.get());

	if (!persInfo)
		return;

	VkViewVre3D* pView3d = dynamic_cast<VkViewVre3D*>(m_view);
	if (!pView3d)
		return;
	auto* pCamera = pView3d->getCamera();

	idea::Point3d pt;

	if (m_panPos.has_value())
	{
		pt = m_panPos.value();
	}
	else
	{
		VkPickResult result = m_picker->pickPoint(oldPos.x(), oldPos.y());
		if (result.valid())
			pt = result.getWorldIntersection().ToPoint();
		else
			pView3d->projectToPlane(oldPos, pView3d->getCurrentPlane(), pt);

		VK_WARN << "\n\n\n\n--------PICKED\n";
		m_panPos = pt;
	}
#endif

#if 0
	double px = pt.X(), py = pt.Y(), pz = pt.Z();

	idea::Matrix4 mat = VkConvertUtils::toIdeaMatrix4(pCamera->getWindowMatrix()) * VkConvertUtils::toIdeaMatrix4(
			pCamera->getProjectionMatrix()) *
		VkConvertUtils::toIdeaMatrix4(pCamera->viewMatrix());

	double m11 = mat(0, 0), m12 = mat(0, 1), m13 = mat(0, 2), m14 = mat(0, 3);
	double m21 = mat(1, 0), m22 = mat(1, 1), m23 = mat(1, 2), m24 = mat(1, 3);
	// double m31 = mat(2, 0), m32 = mat(2, 1), m33 = mat(2, 2), m34 = mat(2, 3);
	double m41 = mat(3, 0), m42 = mat(3, 1), m43 = mat(3, 2), m44 = mat(3, 3);

	double sx = newPos.x(), sy = pView3d->getSize().height() - newPos.y();
	auto dir = m_cameraInfo->getViewDir();
	Normalize(dir);

	double dirX = dir.X(), dirY = dir.Y(), dirZ = dir.Z();

	double A = dirX * px + dirY * py + dirZ * pz;
	double B = sx * m44 - m14;
	double C = sy * m44 - m24;

	idea::Matrix3 M(dirX, dirY, dirZ, m11 - sx * m41, m12 - sx * m42, m13 - sx * m43, m21 - sy * m41, m22 - sy * m42,
	                m23 - sy * m43);
	M = M.Inverse();
	idea::Vector3d v(A, B, C);
	idea::Vector3d out = mul(M, v);
	double a = out.X(), b = out.Y(), c = out.Z();

	double dx = out.X() - px;
	double dy = out.Y() - py;
	double dz = out.Z() - pz;
#endif


#if 0
	idea::Vector3d offset(dx, dy, dz);
	idea::Point3d ptt = pt + idea::Vector3d(dx, dy, dz);
	auto ptts = ptt.Transformed(mat) - idea::Vector3d(sx, sy, 0);

	double aa1 = dirX * a + dirY * b + dirZ * c - A;
	double bb1 = m11 * a + m12 * b + m13 * c - B;
	double cc1 = m21 * a + m22 * b + m23 * c - C;

	VK_WARN << "--------OFFSET:   " << dx << "  , " << dy << "   " << dz << "    NEW-POS: " << newPos.x() << "  , " <<
		newPos.y() << "\n";
	VK_WARN << "--------DELTA: " << ptts.ToString() << "  ,  DOT: " << offset.Dot(dir) << "\n";
#endif

#if 0
	auto newEye = m_cameraInfo->getPos() - Vector3d(dx, dy, dz);
	m_cameraInfo->setPos(newEye);

	double distance = persInfo->getDistance();
	auto newCenter = newEye + distance * dir;
	m_cameraInfo->setCenter(newCenter);
	updateCameraByScene();
#endif
}

#if 0
float getNearDistance(osg::Camera* camera, osg::BoundingBox& bb, float dist)
{
	float delta = dist * .02;
	float preDist = dist;
	int COUNT = 0;

	while (true)
	{
		osg::Matrix view_m = osg::Matrixd::translate(-bb.center()) * osg::Matrixd::rotate(
			camera->getViewMatrix().getRotate()) * osg::Matrixd::translate(0, 0, -dist);

		osg::Matrix proj = camera->getProjectionMatrix();

		float w, h;
		w = camera->getViewport()->width();
		h = camera->getViewport()->height();

		for (int i = 0; i <= 7; i++)
		{
			osg::Vec3 pp = bb.corner(i) * view_m * camera->getProjectionMatrix() * camera->getViewport()->
				computeWindowMatrix();

			if (pp.x() < 0 || pp.x() > w || pp.y() < 0 || pp.y() > h)
			{
				dist += delta;
				return std::min(dist, preDist);
			}
		}

		dist -= delta;
		COUNT++;
		if (COUNT >= 100 || dist < 0) return preDist;
	}

	return preDist;
}

void ThreeDimManipulator::focus(dtData* obj)
{
	//-------------------------------------------------- 获取包围盒
	osg::BoundingSphere bs;
	osg::BoundingBox bb;
	if (obj && obj->as_dtObject())
	{
		THObject* var = obj->as_dtObject()->object();
		if (var && var->asOSGObject())
		{
			osg::ComputeBoundsVisitor cbVisitor;
			var->asOSGObject()->accept(cbVisitor);

			osg::BoundingBox& bb = cbVisitor.getBoundingBox();

			if (bb.valid())
			{
				bs.expandBy(bb);
			}
			else
			{
				bs = var->asOSGObject()->getBound();
			}

			auto nodePaths = var->asOSGObject()->getParentalNodePaths(); //包含自身
			if (nodePaths.size() == 0 || nodePaths[0].size() == 0) return; //如果节点隐藏时，则nodePaths.size() == 0

			std::vector<osg::Node*> vec = nodePaths[0];
			osg::Matrix mat = vec[0]->getWorldMatrices()[0];

			bs.center() = bs.center() * mat;
		}
	}
	else
	{
		//如果是无效的指针，则全屏居中
		osg::ComputeBoundsVisitor cbVisitor;
		_viewer->graphNode()->accept(cbVisitor);

		bb = cbVisitor.getBoundingBox();

		if (bb.valid())
		{
			bs.expandBy(bb);
		}
		else
		{
			bs = _viewer->graphNode()->getBound();
		}
	}

	//--------------------------------------------------透视模式
	if (_viewer->isPerspectiveCamera())
	{
		double dist = 3.5f * bs.radius();

		osg::Camera* camera = _viewer->getCamera();

		if (camera)
		{
			// try to compute dist from frustum
			double left, right, bottom, top, zNear, zFar;
			if (camera->getProjectionMatrixAsFrustum(left, right, bottom, top, zNear, zFar))
			{
				double vertical2 = fabs(right - left) / zNear / 2.;
				double horizontal2 = fabs(top - bottom) / zNear / 2.;
				double dim = horizontal2 < vertical2 ? horizontal2 : vertical2;
				double viewAngle = atan2(dim, 1.);
				dist = bs.radius() / sin(viewAngle);
			}
		}

		osgGA::OrbitManipulator* mani = dynamic_cast<osgGA::OrbitManipulator*>(_viewer->getCameraManipulator());

		if (mani)
		{
			_center = bs.center();
			dist = getNearDistance(camera, bb, dist);
			_distance = dist;
		}
	}
	else //--------------------------------------------------正交模式
	{
		_center.set(bs.center());

		const double viewPortW = _viewer->getCamera()->getViewport()->width();
		const double viewPortH = _viewer->getCamera()->getViewport()->height();

		_zoomFactor = 1.8f * bs.radius() / std::min(viewPortW, viewPortH);
		if (_zoomFactor <= 0)
			_zoomFactor = 0.001;

		const float w = viewPortW * _zoomFactor;
		const float h = viewPortH * _zoomFactor;

		_viewer->getCamera()->setProjectionMatrixAsOrtho2D(-w / 2, w / 2, -h / 2, h / 2);
	}
}

void ThreeDimManipulator::rectZoomPersp()
{
	osg::Vec2 pt1, pt2;
	{
		bool bHandled = false;
		QCursor preCursor = _viewer->cursor();

		_viewer->setCursor(g::RectZoom);
		msg::send(_viewer->eventHandlerID(), _msg(add_hud_rect));

		do
		{
			pt1 = CommandIO::getMousePosition("指定第一个点:");
			if (CommandIO::haveEscapePressed()) break;

			pt2 = CommandIO::getMousePosition("指定第二个点:");
			if (CommandIO::haveEscapePressed()) break;

			bHandled = true;
		}
		while (0);

		_viewer->setCursor(preCursor);

		msg::send(_viewer->eventHandlerID(), _msg(remove_hud_rect));
		if (!bHandled) return;
	}

	double wx, wy;
	_viewer->getWindowSize(wx, wy);

	//映射到[-1, 1]
	pt1 = osg::Vec2(pt1.x() / wx * 2 - 1, pt1.y() / wy * 2 - 1);
	pt2 = osg::Vec2(pt2.x() / wx * 2 - 1, pt2.y() / wy * 2 - 1);

	osg::Camera* camera = _viewer->getCamera();

	osg::ref_ptr<osgUtil::IntersectorGroup> intersectors = new osgUtil::IntersectorGroup;

	intersectors->addIntersector(
		new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, (pt1.x() + pt2.x()) / 2,
		                                    (pt1.y() + pt2.y()) / 2));
	intersectors->addIntersector(
		new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, pt1.x(), pt1.y()));
	intersectors->addIntersector(
		new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, pt1.x(), pt2.y()));
	intersectors->addIntersector(
		new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, pt2.x(), pt1.y()));
	intersectors->addIntersector(
		new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, pt2.x(), pt2.y()));

	const float SAMPLE_DELTA = 1.f / 15.f;
	const float kDELTA = 0.05;
	for (float i = kDELTA; i <= 1 - kDELTA; i += SAMPLE_DELTA)
	{
		for (float j = kDELTA; j <= 1 - kDELTA; j += SAMPLE_DELTA)
		{
			osg::Vec2 mid(pt1.x() * (1 - i) + pt2.x() * i, pt1.y() * (1 - j) + pt2.y() * j);
			intersectors->addIntersector(
				new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, mid.x(), mid.y()));
		}
	}

	osgUtil::IntersectionVisitor iv(intersectors.get());
	iv.setTraversalMask(~(g::NM_GRID | g::NM_BACK_GROUND));
	camera->accept(iv);

	osg::Vec3 POSs;
	vector<osg::Vec3> vecPos;
	for (osg::ref_ptr<osgUtil::Intersector>& is : intersectors->getIntersectors())
	{
		osgUtil::LineSegmentIntersector* lis = dynamic_cast<osgUtil::LineSegmentIntersector*>(is.get());
		if (lis && lis->containsIntersections())
		{
			osg::Vec3 pos = lis->getFirstIntersection().getWorldIntersectPoint();
			POSs += pos;
			vecPos << pos;
		}
	}

	//LOG_INFO << "POS CNT " << vecPos.size();

	if (vecPos.size() == 0)
	{
		focus(nullptr);
	}
	else
	{
		float dis = FLT_MIN;
		for (int i = 0; i < vecPos.size(); i++)
		{
			for (int j = i + 1; j < vecPos.size(); j++)
			{
				dis = std::max(dis, (vecPos[i] - vecPos[j]).length2());
			}
		}

		_center = POSs / vecPos.size();
		_distance = std::sqrt(dis) * std::cos(osg::DegreesToRadians(30.f));
	}
}

void ThreeDimManipulator::toggleCameraModel()
{
	double wx, wy;
	_viewer->getWindowSize(wx, wy);

	vector<osg::Vec3> vecPos;
	vector<osg::Vec3> screenPos;

	std::default_random_engine generator(time(NULL));
	std::uniform_real_distribution<double> distribution(-1., 1.);

	osg::Camera* camera = _viewer->getCamera();

	osg::ref_ptr<osgUtil::IntersectorGroup> intersectors = new osgUtil::IntersectorGroup;

	for (int i = 0; i < 50; i++)
	{
		double x = distribution(generator);
		double y = distribution(generator);

		intersectors->addIntersector(new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, x, y));
	}

	osgUtil::IntersectionVisitor iv(intersectors.get());
	iv.setTraversalMask(~(g::NM_GRID | g::NM_BACK_GROUND));
	camera->accept(iv);

	for (osg::ref_ptr<osgUtil::Intersector>& is : intersectors->getIntersectors())
	{
		osgUtil::LineSegmentIntersector* lis = dynamic_cast<osgUtil::LineSegmentIntersector*>(is.get());
		if (lis && lis->containsIntersections())
		{
			osg::Vec3 pos = lis->getFirstIntersection().getWorldIntersectPoint();
			osg::NodePath np = lis->getFirstIntersection().nodePath;

			osg::Vec3 screenPt = util::wordToScreen(camera, pos);
			if (screenPt.x() >= 0 && screenPt.x() <= wx && screenPt.y() >= 0 && screenPt.y() <= wy)
			{
				vecPos << pos;
				screenPos << screenPt;
			}
		}
	}


	if (vecPos.size() == 0) //没求到交点，则用包围盒
	{
		osg::ComputeBoundsVisitor cbVisitor;
		cbVisitor.setTraversalMask(~(g::NM_GRID | g::NM_BACK_GROUND));
		_viewer->graphNode()->accept(cbVisitor);
		osg::BoundingBox& bb = cbVisitor.getBoundingBox();
		for (int i = 0; i <= 7; i++)
		{
			osg::Vec3 pos = bb.corner(i);
			vecPos << pos;
			osg::Vec3 screenPt = util::wordToScreen(camera, pos);
			screenPos << screenPt;
		}
	}

	if (_viewer->isPerspectiveCamera()) //--------------------------------------------------透视to正交
	{
		_viewer->setCameraModel(!_viewer->isPerspectiveCamera());

		float right = 0;
		int p2_0 = 0;

		for (int i = 0; i < vecPos.size(); i++)
		{
			//screenPos[i] == vecPos[i] * mat_v * PROJ * camera->getViewport()->computeWindowMatrix();
			//p1 * PROJ = p2
			osg::Vec4 p1 = osg::Vec4(vecPos[i], 1) * camera->getViewMatrix();
			osg::Vec4 p2 = osg::Vec4(screenPos[i], 1) * osg::Matrix::inverse(
				camera->getViewport()->computeWindowMatrix());

			if (p2.x() == 0)
			{
				p2_0++;
				continue;
			}

			right += p1.x() / p2.x();
			//LOG_INFO << "AA\t" << p1.x() / p2.x();
		}

		if (right < 0)
		{
			//LOG_INFO << "aa";
		}

		//if (p2_0 == vecPos.size()) return;

		right /= vecPos.size();
		_viewer->getCamera()->setProjectionMatrixAsOrtho2D(-right, right, -right * wy / wx, right * wy / wx);
	}
	else //--------------------------------------------------正交to透视
	{
		_viewer->setCameraModel(!_viewer->isPerspectiveCamera());
		float DIS = 0;
		for (int i = 0; i < vecPos.size(); i++)
		{
			//screenPos[i] == vecPos* osg::Matrixd::translate(-_center) * osg::Matrixd::rotate(_rotation.inverse()) *
			//osg::Matrixd::translate(0.0, 0.0, -XX) * camera->getProjectionMatrix() * camera->getViewport()->computeWindowMatrix();
			osg::Vec4 p1 = osg::Vec4(vecPos[i], 1) * osg::Matrixd::translate(-_center) * osg::Matrixd::rotate(
				_rotation.inverse());
			osg::Vec4 p2 = osg::Vec4(screenPos[i], 1) * osg::Matrix::inverse(
				camera->getProjectionMatrix() * camera->getViewport()->computeWindowMatrix());
			//p1 * M = p2

			float dist = -p1.x() / p2.x() * p2.z() + p1.z();

			//LOG_INFO << "P1P2 " << p1 << "\t" << p2 << "\t" <<  osg::componentDivide(p1, p2) << "\t"<< dist;

			if (dist < 0)
			{
				//LOG_INFO << "bb";
			}
			//LOG_INFO << "BB\t" << dist;
			DIS += dist;
		}
		DIS /= vecPos.size();
		_distance = DIS;
	}
}

void ThreeDimManipulator::change_pos_from_little_map(msg::base& m)
{
	using type = msg::common2<bool, float, osg::Vec3>*;
	type t = dynamic_cast<type>(&m);
	if (t)
	{
		bool have_interscetion = std::get<0>(t->param);
		float dis = std::get<1>(t->param);
		osg::Vec3 intersect_pos = std::get<2>(t->param);

		if (have_interscetion)
			_center = intersect_pos;
		else
			_center = osg::Vec3(intersect_pos.x(), intersect_pos.y(), _center.z());

		if (_viewer->isPerspectiveCamera())
		{
			_distance = dis;
		}
		else
		{
			_distance = 1000;
			const double viewPortW = _viewer->getCamera()->getViewport()->width();
			const double viewPortH = _viewer->getCamera()->getViewport()->height();

			float w = 100;
			float h = w * viewPortH / viewPortW;
			_viewer->getCamera()->setProjectionMatrixAsOrtho2D(-w / 2, w / 2, -h / 2, h / 2);
		}
	}
}

void ThreeDimManipulator::rectZoomOrth()
{
	osg::Vec2 pt1, pt2;
	{
		bool bHandled = false;
		Qt::CursorShape preCursor = _viewer->cursor().shape();
		_viewer->setCursor(g::RectZoom);
		msg::send(_viewer->eventHandlerID(), _msg(add_hud_rect));

		do
		{
			pt1 = CommandIO::getMousePosition("指定第一个点:");
			if (CommandIO::haveEscapePressed()) break;

			pt2 = CommandIO::getMousePosition("指定第二个点:");
			if (CommandIO::haveEscapePressed()) break;

			bHandled = true;
		}
		while (0);

		_viewer->setCursor(g::MouseCursor(preCursor));
		msg::send(_viewer->eventHandlerID(), _msg(remove_hud_rect));
		if (!bHandled) return;
	}

	osg::Camera* camera = _viewer->getCamera();
	_center = util::screenToWorld(camera, osg::Vec3((pt1 + pt2) / 2, 0));

	//--------------------------------------------------调整投影矩阵
	double x, y;
	_viewer->getWindowSize(x, y);

	double left, right, bottom, top, zNear, zFar;
	_viewer->getCamera()->getProjectionMatrixAsOrtho(left, right, bottom, top, zNear, zFar);

	double f = std::max(std::abs(pt1.x() - pt2.x()) / x, std::abs(pt1.y() - pt2.y()) / y);

	_viewer->getCamera()->setProjectionMatrixAsOrtho2D(left * f, right * f, bottom * f, top * f);

	//--------------------------------------------------更改视图矩阵
	osg::Matrix mat = osg::Matrixd::translate(-_center) * osg::Matrixd::rotate(_rotation.inverse()) *
		osg::Matrix::translate(0, 0, 1);
	camera->setViewMatrix(mat);
}
#endif
